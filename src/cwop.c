/* cwop.c */

/* Citizen Weather / APRS reporting for Oww */
/*  see README.CWOP for setup information */

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <config.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
extern int close(int fd) ;
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#include <sys/types.h>
//static pid_t cwop_proc = 0;
#endif

#include <math.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <string.h>

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
# ifdef TIME_WITH_SYS_TIME
#  include <time.h>
# endif
#else
# include <time.h>
#endif

#include <sys/select.h>
#include <errno.h>
#include <sys/errno.h>
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#ifdef WIN32
#  include <winsock.h>
#else
#include <sys/socket.h>
#ifdef RISCOS
#  include <sys/byteorder.h>
#endif
#endif

#include "globaldef.h"
#include "setup.h"

#include "werr.h"
#include "convert.h"
#include "devices.h"
#include "rainint.h"
#include "stats.h"
#include "weather.h"
#include "intl.h"
#include "utility.h"

extern time_t the_time;

#define BUF_SIZE        256
#define CWOP_DEBUG WERR_DEBUG0
#define INVALID_SOCKET -1

/* Enumeration for cwop state machine */

enum Cwop_States
{
  Cwop_Start = 0,  // Create the Wx string, get the IP
  Cwop_Select,     // Poll for write access to see if connexion established
  Cwop_Init,       // Read initial string
  Cwop_Login,      // Write login string
  Cwop_LoginResp,  // Read login response
  Cwop_Wx,         // Write Wx string
  Cwop_WxResp,     // Read Wx response
  Cwop_Idle        // Nothing to do
} ;

static time_t cwop_start_time = 0 ;
static int cwop_state = Cwop_Idle ;
static int fd = INVALID_SOCKET ; // The socket

// Close any existing Cwop session
static void
cwop_close(void)
{
  if (fd != INVALID_SOCKET)
  {
    close(fd) ;
    fd = INVALID_SOCKET ;
  }

  cwop_state = Cwop_Idle ;
  cwop_start_time = 0 ;

  return ;
}

/*
  Create the data string to send.  Should look something like
    KG4QXL>APRS,TCPIP*:@032052z3852.76N/07823.45W_157/000g000t080
*/
static void
getCwopWx (char *buf, statsmean * wx)
{
        struct tm *gmt;
  float lat_min, lon_min;
  int sensor, tempVal;
  char tempBuf[BUF_SIZE];

  char *saved_locale;

  gmt = gmtime (&the_time);

  lat_min = (float) fabs (ws.latitude - (int) ws.latitude) * 60.0F;
  lon_min = (float) fabs (ws.longitude - (int) ws.longitude) * 60.0F;

  /*
  * this is all kind of pointless without a temp gauge of some kind
  */
  if (!devices_have_something ())
  {
                buf = '\0';
    return;
  }

  /* Temporarily switch to POSIZ locale so that we are sure to get decimal points */

  /* Get a copy of the name of the current locale. */
  saved_locale = strdup(setlocale(LC_NUMERIC, NULL));
  
  /* If that worked change to POSIX */
  if (saved_locale != NULL)
  {
    setlocale (LC_NUMERIC, "POSIX");
  }

  /*
  * construct the header and minimal weather data
  *  Note: oww stores lat / lon relative to the N/E quadrant
  */
  sprintf (buf,
    "%s>APRS,TCPIP*:@%02d%02d%02dz%02d%05.2f%c/%03d%05.2f%c_"
    "%03d/%03dg%03dt%03d",
    setup_cwop_user,
    gmt->tm_mday, gmt->tm_hour, gmt->tm_min,
    abs ((int) ws.latitude), lat_min,
    ((ws.latitude < 0.0F) ? 'S' : 'N'),
    abs ((int) ws.longitude), lon_min,
    ((ws.longitude < 0.0F) ? 'W' : 'E'),
    (int) wx->meanWd,
    (int) convert_speed (wx->meanWs, CONVERT_MPH),
    (int) convert_speed (wx->maxWs, CONVERT_MPH),
    (int) convert_temp (weather_primary_T (wx),
            CONVERT_FAHRENHEIT));

  /*
  * if we have a rain sensor then add it in there
  *  choices are:
  *   r, rainfall (in hundredths of an inch) in the last hour
  *   p, rainfall (in hundredths of an inch) in the last 24 hours
  *   P, rainfall (in hundredths of an inch) since midnight
  */
  if (devices_have_rain () && (wx->rain >= 0))
  {
	// Rainfall in last hour
    tempVal = (int) (100 * wx->rainint1hr);
    if (tempVal > 999) tempVal = 999;

    sprintf (tempBuf, "r%03d", tempVal);
    strcat (buf, tempBuf);

    // Rainfall in last 24 hours
    tempVal = (int) (100 * wx->rainint24hr);
    if (tempVal > 999) tempVal = 999;

    sprintf (tempBuf, "p%03d", tempVal);
    strcat (buf, tempBuf);

    // Rainfall today
    tempVal = (int) (100.0F * wx->dailyrain);
    if (tempVal > 999) tempVal = 999;

    sprintf (tempBuf, "P%03d", tempVal);
    strcat (buf, tempBuf);
  }

  /*
  * if we have a hygrometer then add it in there
  */
  if ((sensor = weather_primary_rh (wx)) != -1)
  {
    tempVal = (int) wx->meanRH[sensor];

    /* if humidity is 100% then APRS wants 00 */
    if (tempVal == 100)
      tempVal = 0;

    sprintf (tempBuf, "h%02d", tempVal);
    strcat (buf, tempBuf);
  }

  /*
  * if we have a barometer then add it in there
  *  hmm, document says 4 places but shows 5.  we'll assume 5
  */
  if ((sensor = weather_primary_barom (wx)) != -1)
  {
    /* tenths of millibars (hPascals) */
    tempVal = (int) (wx->meanbarom[sensor] * 10);

    sprintf (tempBuf, "b%05d", tempVal);
    strcat (buf, tempBuf);
  }

  strcat (buf, "lOww_0.86.5\r\n");

  if (saved_locale != NULL)
  {
    /* Restore the original locale. */
    setlocale (LC_ALL, saved_locale);
    free (saved_locale);
  }
  
  return;
}

static int
cwop_poll_write(void)
{
  struct timeval select_time = {0, 10} ;
  fd_set writefds, ers ;

  FD_ZERO(&writefds) ;
  FD_SET(fd, &writefds) ;

  FD_ZERO(&ers) ;
  FD_SET(fd, &ers) ;

  select(fd+1, NULL, &writefds, &ers, &select_time) ;

  /* Error condition? */
  if (FD_ISSET(fd, &ers))
  {
    werr(WERR_WARNING, _("Cwop: Remote host error")) ;
    cwop_close() ;
    return -1 ;
  }

  /* Writeable? */

  if (FD_ISSET(fd, &writefds))
  {
    return 1 ; // Writeable
  }

  return 0 ; // Nothing doing
}

static int
cwop_poll_read(void)
{
  struct timeval select_time = {0, 10} ;
  fd_set readfds, ers ;

  FD_ZERO(&readfds) ;
  FD_SET(fd, &readfds) ;

  FD_ZERO(&ers) ;
  FD_SET(fd, &ers) ;

  select(fd+1, &readfds, NULL, &ers, &select_time) ;

  /* Error condition? */
  if (FD_ISSET(fd, &ers))
  {
    werr(WERR_WARNING, _("Cwop: Remote host error")) ;
    cwop_close() ;
    return -1 ;
  }

  /* Readable? */

  if (FD_ISSET(fd, &readfds))
  {
    return 1 ; // Readable
  }

  return 0 ; // Nothing doing
}

static int
cwop_state_machine (statsmean * wx)
{
  static struct sockaddr_in svrAddr;	/* server's address */
  static struct hostent *hostInfo;
  struct in_addr inaddr;
  static char readBuf[BUF_SIZE], writeBuf[BUF_SIZE], wxString[BUF_SIZE] ;
	unsigned long a = 1L ;

  // -------------------------

  switch(cwop_state)
  {
    case Cwop_Idle:
      // Nothing to do
      return 0 ;
      break ;

    case Cwop_Start: // Start up cwop session

      werr (CWOP_DEBUG, "CWOP session starting");
      memset ((void *) readBuf, 0, BUF_SIZE);

      /*
      * Build wxString - do it now, as wx might have changed by the time we connect
      */
      getCwopWx (wxString, wx);

      /*
      * no real point if there's no data to send
      */
      if (!strlen (wxString))
      {
        //close (fd);
        return (-1);
      }

      /*
      * set this up as a tcp connection to the cwop port
      */
      svrAddr.sin_family = AF_INET;
      svrAddr.sin_port = htons (setup_cwop_port);

      /*
      * check to see whether the hostname is an IP or actual name
      */
      if (inet_aton(setup_cwop_server, &inaddr) != 0)
      {
        memcpy ((void *) &svrAddr.sin_addr, (void *) &inaddr,
              sizeof (inaddr));
      }
      else
      {
        if ((hostInfo = gethostbyname (setup_cwop_server)) == NULL)
        {
          werr (WERR_WARNING + WERR_AUTOCLOSE, _("Cwop: Host name error: %s"),
                setup_cwop_server);
          return (-1);
        }

        memcpy ((void *) &svrAddr.sin_addr, hostInfo->h_addr,
              hostInfo->h_length);
      }

      /*
      * create the socket
      */
      if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
      {
        werr (WERR_WARNING + WERR_AUTOCLOSE, _("Cwop: Cannot create tcp socket - %s"), strerror(errno));
        fd = INVALID_SOCKET ;
        return (-1);
      }

      /*
      * Make the socket non-blocking
      */

      ioctl(fd, FIONBIO, &a) ;

      /*
      * connect to the server
      */
      if (connect (fd, (struct sockaddr *) &svrAddr, sizeof (svrAddr)) < 0)
      {
        if (errno != EINPROGRESS)
        {
          werr (WERR_WARNING + WERR_AUTOCLOSE,
            _("Cwop: Cannot connect to server %s - %s"),
            setup_cwop_server,
            strerror(errno));
          return (-1);
        }
        else
        {
          werr(CWOP_DEBUG, "Cwop: connect() returned EINPROGRESS") ;
        }
      }
      else
      {
        werr(CWOP_DEBUG, "Cwop: connect() worked straight off") ;
      }

      /*
      * Make the socket blocking
      */

      //ioctl(fd, FIONBIO, &b) ;

      cwop_state = Cwop_Select ;
      return 0 ;

    case Cwop_Select:
      /* The socket has become writeable - we may continue */

      werr(CWOP_DEBUG, "Cwop: socket has become writeable") ;

      cwop_state = Cwop_Init ;
      return 0 ;

    case Cwop_Init: // Read the initial connection string from the server

    /*
      * read the initial connection string from the server
      */
      if (recv(fd, (char *) readBuf, BUF_SIZE, 0) != -1)
      {
        werr (CWOP_DEBUG, "Cwop: %s", readBuf);
        memset ((void *) readBuf, 0, BUF_SIZE);
      }
      else
        werr(CWOP_DEBUG, "Cwop: read() failed - %s", strerror(errno)) ;

      cwop_state = Cwop_Login ;
      return 0 ;

    case Cwop_Login: // write login string

      /*
      * send the login information
      */
      sprintf (writeBuf, "user %s pass %s vers %s\r\n",
    	  setup_cwop_user, setup_cwop_pass, "Oww " VERSION
    #           ifdef RISCOS
        "-RO"
    #           else
        "-L"
    #           endif
        );

      if (send(fd, (char *) writeBuf, strlen (writeBuf), 0) != -1)
      {
        werr (CWOP_DEBUG, "Cwop: %s", writeBuf);
      }
      else
        werr(CWOP_DEBUG, "Cwop: write() failed - %s", strerror(errno)) ;

      cwop_state = Cwop_LoginResp ;
      return 0 ;

    case Cwop_LoginResp: // read login response

      /*
      * read the login response
      */
      if (recv(fd, (char *) readBuf, BUF_SIZE, 0) != -1)
      {
        werr (CWOP_DEBUG, "Cwop: %s", readBuf);
        memset ((void *) readBuf, 0, BUF_SIZE);
      }
      else
        werr(CWOP_DEBUG, "Cwop: read() failed - %s", strerror(errno)) ;

      cwop_state = Cwop_Wx ;
      return 0 ;

    case Cwop_Wx: // write Wx string

      /*
      * send the weather data
      */
      if (send(fd, (char *) wxString, strlen (wxString), 0) != -1)
      {
        werr (CWOP_DEBUG, "Cwop: %s", wxString);
      }
      else
        werr(CWOP_DEBUG, "Cwop: write() failed - %s", strerror(errno)) ;

      cwop_state = Cwop_WxResp ;
      return 0 ;

    case Cwop_WxResp: // read Wx response

      /*
      * read the response
      */
      if (recv(fd, (char *) readBuf, BUF_SIZE, 0) != -1)
      {
        werr (CWOP_DEBUG, "Cwop: %s", readBuf);
        memset ((void *) readBuf, 0, BUF_SIZE);
      }
      else
        werr(CWOP_DEBUG, "Cwop: read() failed - %s", strerror(errno)) ;

      cwop_close();
      return 0 ;
  }

  // -------------------------

  return 0;		/* Ok */
}

int
cwop_send (statsmean * wx)
{
  // Don't start a session if already running

  if (cwop_state != Cwop_Idle) return 0 ;

  cwop_state = Cwop_Start ;

  cwop_start_time = time(NULL) ;

  if (-1 == cwop_state_machine(wx))
  {
    cwop_close() ;
    return -1 ;
  }

  return 0 ;
}

int
cwop_poll (void)
{
  if (cwop_state == Cwop_Idle) return 0 ;

  if (fd == INVALID_SOCKET)
  {
    werr(CWOP_DEBUG, "Cwop: Socket is invalid") ;
    cwop_close() ;
    return 0 ;
  }

  if (cwop_start_time == 0)
  {
    werr(CWOP_DEBUG, "Cwop: Timer not started!") ;

    cwop_close() ;
    return 0 ;
  }


  if (time(NULL) > cwop_start_time+CWOP_TIMEOUT)
  {
    werr(CWOP_DEBUG, "Cwop: Timeout") ;

    cwop_close() ;
    return 0 ;
  }

  //werr(CWOP_DEBUG, "cwop_state = %d", cwop_state) ;

  switch (cwop_state)
  {
    //case Cwop_Idle:
    //  return 0 ;

    case Cwop_Start:
      werr(CWOP_DEBUG, "cwop_poll() called in Cwop_Start state") ;
      cwop_close() ;
      return 0 ;

    // These cases need to be able to write data
    case Cwop_Select:
    case Cwop_Login:
    case Cwop_Wx:
    {
      switch(cwop_poll_write())
      {
        case -1:
          cwop_close() ;
          return 0 ;

        case 0:
          return 1 ; // Session running, but must wait

        case 1:
          werr(CWOP_DEBUG, "Cwop: Ready to write") ;
      }
      break ;
    }

    // These cases need to be able to read data
    case Cwop_Init:
    case Cwop_LoginResp:
    case Cwop_WxResp:
    {
      switch(cwop_poll_read())
      {
        case -1:
          cwop_close() ;
          return 0 ;

        case 0:
          return 1 ; // Session running, but must wait

        case 1:
          werr(CWOP_DEBUG, "Cwop: Ready to read") ;
      }
      break ;
    }
  }

  // Ready for real call to state machine now
  if (-1 == cwop_state_machine(NULL))
    cwop_close() ;

  return (cwop_state != Cwop_Idle) ;
}
