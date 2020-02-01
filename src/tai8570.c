/* tai8570.c

  Readout of AAG TAI 8570 barometer

  Simon J. Melhuish, 2003

  For Oww
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ownet.h"
#include "werr.h"
#include "devices.h"
#include "wstypes.h"
#include "swt12.h"
#include "applctn.h"
#include "intl.h"

// Pressure_Sensor_Utility
//
//
//
// Consts
//
//  DS2407_Family  = $12
//
//  File1WireName  = '8570';
//  File1WireExt   =  0;
//
//

/* Constants */

#define SEC_RESET  "101010101010101000000"	// Reset command sequence
#define SEC_READW1 "111010101000"	// READ W1 command sequence
#define SEC_READW2 "111010110000"	// READ W2 command sequence
#define SEC_READW3 "111011001000"	// READ W3 command sequence
#define SEC_READW4 "111011010000"	// READ W4 command sequence
#define SEC_READD1  "11110100000"	// READ D1 command sequence
#define SEC_READD2  "11110010000"	// READ D2 command sequence

#define CHANNEL_ACCESS 0xF5
#define CFG_READ       0xec	// Read configuration for DS2407
#define CFG_WRITE      0x8c	// Write configuration for DS2407
#define CFG_RPULSE     0xc8	// Read pulse configuration for DS2407


/* Local function declarations */

static int SendPressureBit (int portnum, int value);
static int ReadPressureBit (int portnum);

static int WritePressureSensor (int device_r, int device_w, char *Command);
static int OpenPIOS_A (int device_r, int device_w);
static int OpenPIOS_B (int device_r, int device_w);
static void ReadPressureSensor (int device_r, int CountBit, char buffer[], int buflen);
static int PressureReset (int device_r, int device_w);
static char *ReadSensorValueStr (int device_r, int device_w, char command[]);
static int ConfigWrite (int device);
static int ConfigRead (int device);
static int ConfigReadPulse (int device);
//static int PrepPIOS2Read (void);
//static int PrepPIOS2Write (void);
static int CheckConversion (int device_r);

static int
ReadSupplyIndicator(int device)
{
  int info ;

  info = ReadSwitch12 (device, 0);

  if (info == -1)
  {
    werr(WERR_WARNING+WERR_AUTOCLOSE, "%s ReadSwitch12 failed", devices_list[device].menu_entry) ;
    return 0 ;
  }

  werr(WERR_DEBUG2, "%s info byte 0x%x", devices_list[device].menu_entry, info) ;

  return ((info & 0x80) != 0) ;
}

static int
first_writer_needed(void)
{
  int i ;

  for (i=0; i<MAXBAROM; ++i)
  {
    if (devices_list[i+devices_BAR1].id[0] == SBATTERY_FAMILY)
      continue ; // Ignore DS2438-based barometers

    if (devices_list[i+devices_BAR1].id[0] == SWITCH_FAMILY)
    {
      if (devices_list[i+devices_tai8570w1].id[0] != SWITCH_FAMILY)
        return i+devices_tai8570w1 ; // No switch allocated

      if (!ReadSupplyIndicator(i+devices_tai8570w1))
        return i+devices_tai8570w1 ; // Switch allocated, but no supply => reader
    }
  }

  return -1 ;
}

/* Go through all the allocated readers, looking for writers */

int
tai8570_check_alloc(void)
{
  int i, wd ;

  for (i=0; i<MAXBAROM; ++i)
  {
    switch(devices_list[i+devices_BAR1].id[0])
    {
      case SWITCH_FAMILY:
        // Check reader
        if (ReadSupplyIndicator(i+devices_BAR1))
        {
          wd = first_writer_needed() ;
          if (-1 == wd)
          {
            werr(WERR_DEBUG0, "No writer slot for %s",
            devices_list[i+devices_BAR1].menu_entry) ;
          }
          else
          {
            werr(WERR_DEBUG0, "%s is not a TAI8570 read device - allocating to %s",
              devices_list[i+devices_BAR1].menu_entry,
              devices_list[wd].menu_entry) ;
            devices_allocate(wd, devices_list[i+devices_BAR1].back_alloc) ;

            /* Clear the old  allocation */
            devices_list[i+devices_BAR1].back_alloc = -1 ;
            devices_list[i+devices_BAR1].id[0] = '\0' ;
            break ;
          }
        }
        if ((devices_list[i+devices_BAR1].calib[0] > 2.0F) ||
            (devices_list[i+devices_BAR1].calib[0] < 0.5F))
        {
          werr(WERR_WARNING+WERR_AUTOCLOSE,
            _("Setting %s slope to default for sea level"),
            devices_list[i+devices_BAR1].menu_entry) ;
          devices_list[i+devices_BAR1].calib[0] = 1.0F ;
        }
        break ;

      case SBATTERY_FAMILY:

      case 0:
      default:
        // Not allocated
      break ;
    }
  }

  return 0 ; // Ok
}

int
tai8570_read_cal(int device_r, int device_w, int cal[])
{
        int i;
        int im = 0;
  char Sw1[20] = "", Sw2[20] = "", Sw3[20] = "", Sw4[20] = "";

  /* Check supply bits */

  /* Write sensor is powered, read senso unpowered */

  if (!ReadSupplyIndicator(device_w))
    werr(WERR_WARNING+WERR_AUTOCLOSE,
    _("%s is not a TAI8570 write device"),
    devices_list[device_w].menu_entry) ;

  if (ReadSupplyIndicator(device_r))
    werr(WERR_WARNING+WERR_AUTOCLOSE,
    _("%s is not a TAI8570 read device"),
    devices_list[device_r].menu_entry) ;

  if (!PressureReset (device_r, device_w))
  {
          werr (WERR_WARNING+WERR_AUTOCLOSE, "ReadSensor_Calibration PressureReset failed");
          return -1 ; // Failure
  }

  strcpy (Sw1, ReadSensorValueStr (device_r, device_w, SEC_READW1));
  strcpy (Sw2, ReadSensorValueStr (device_r, device_w, SEC_READW2));
  strcpy (Sw3, ReadSensorValueStr (device_r, device_w, SEC_READW3));
  strcpy (Sw4, ReadSensorValueStr (device_r, device_w, SEC_READW4));

  if ((strlen (Sw1) != 16) ||
      (strlen (Sw2) != 16) ||
      (strlen (Sw3) != 16) || (strlen (Sw4) != 16))
  {
          werr (WERR_WARNING+WERR_AUTOCLOSE, "ReadSensor_Calibration bad cal string length");
          return -1 ; // Failure
  }

  cal[0] = cal[1] = cal[2] = cal[3] = cal[4] = cal[5] = 0;

  for (i = 14; i >= 0; --i)
  {
          if (Sw1[i] == '1')
                  cal[0] = cal[0] | (1 << (14 - i));
  }

  for (i = 9; i >= 0; --i)
  {
          if (Sw2[i] == '1')
                  cal[4] = cal[4] | (1 << (9 - i));
  }
  if (Sw1[15] == '1')
          cal[4] = cal[4] | (1 << 10);

  for (i = 15; i >= 10; --i)
  {
          if (Sw2[i] == '1')
                  cal[5] = cal[5] | (1 << (15 - i));
  }

  for (i = 9; i >= 0; --i)
  {
          if (Sw3[i] == '1')
                  cal[3] = cal[3] | (1 << (9 - i));
  }

  for (i = 9; i >= 0; --i)
  {
          if (Sw4[i] == '1')
                  cal[2] = cal[2] | (1 << (9 - i));
  }

  for (i = 15; i >= 10; --i)
  {
          if (Sw4[i] == '1')
                  cal[1] = cal[1] | (1 << im);

          ++im;
  }

  for (i = 15; i >= 10; --i)
  {
          if (Sw3[i] == '1')
                  cal[1] = cal[1] | (1 << im);

          ++im;
  }

  //printf ("cal[0]=%d\n", cal[0]);
  //printf ("cal[1]=%d\n", cal[1]);
  //printf ("cal[2]=%d\n", cal[2]);
  //printf ("cal[3]=%d\n", cal[3]);
  //printf ("cal[4]=%d\n", cal[4]);
  //printf ("cal[5]=%d\n", cal[5]);

  return 0 ; // Ok
}


static int
CheckConversion (int device_r)
{
        int i;
        if (!ConfigReadPulse (device_r))
                return FALSE;
        for (i = 0; i < 100; i++)
  {
                if (!owTouchBit (devices_portnum(device_r), 1))
                        break;
    applctn_quick_poll(0) ;
  }
        return (i < 100);
}

static int
ReadSensorValue (int device_r, int device_w, char command[], int *val)
{
        int i, Res = 0;
        static char readbuf[] = "0000000000000000";
        if ((!PressureReset (device_r, device_w)) ||
            (!WritePressureSensor (device_r, device_w, command)) ||
            (!CheckConversion (device_r)) || (!OpenPIOS_A (device_r, device_w)))
        {
                werr (WERR_WARNING+WERR_AUTOCLOSE, "ReadSensorValue failed");
                return -1 ; /* Failure */
        }
        if (!OpenPIOS_A (device_r, device_w))
                return -1 ; /* Failure */
        ReadPressureSensor (device_r, 16, readbuf, sizeof (readbuf));

        if (!OpenPIOS_B (device_r, device_w))
        {
                werr (WERR_WARNING+WERR_AUTOCLOSE, "ReadSensorValue failed");
                return -1 ; /* Failure */
        }

        if (strlen (readbuf) != 16)
        {
                werr (WERR_WARNING+WERR_AUTOCLOSE, "ReadSensorValue strlen(readbuf) = %d",
                      strlen (readbuf));
                return -1 ; /* Failure */
        }

        for (i = 0; i < 16; i++)
        {
                Res = (Res << 1);
                if (readbuf[i] == '1')
                        Res = Res + 1;
        }

  /* Copy over the result */
  *val = Res ;
        return 0 ;
}

static char *
ReadSensorValueStr (int device_r, int device_w, char command[])
{
        static char readbuf[] = "0000000000000000";

        /*if ((!InitBothPIOA()) ||
        * (!WritePressureSensor(command)) ||
        * (!InitBothPIOA())) */
        if (!OpenPIOS_A (device_r, device_w))
                return NULL;
  applctn_quick_poll(0) ;
        if (!WritePressureSensor (device_r, device_w, command))
        {
                werr (WERR_WARNING+WERR_AUTOCLOSE, "ReadSensorValueStr failed");
                return NULL;
        }
  applctn_quick_poll(0) ;
        if (!OpenPIOS_A (device_r, device_w))
                return NULL;
  applctn_quick_poll(0) ;
        ReadPressureSensor (device_r, 16, readbuf, sizeof (readbuf));
        /*if (!InitBothPIOB())
        * {
        * werr(WERR_WARNING+WERR_AUTOCLOSE, "ReadSensorValueStr failed") ;
        * return NULL ;
        * } */
  applctn_quick_poll(0) ;
        OpenPIOS_B (device_r, device_w);

        return readbuf;
}



int
tai8570_read_vals(int device_r, int device_w, int cal[], float *Pressure, float *Temperature)
{
        double DT, SENSE, X, T, Press, OFF;
  int	d1, d2 = 0 ;

        Press = 0.0;
        if (ReadSensorValue (device_r, device_w, SEC_READD1, &d1) < 0)
    return -1 ; // Failure
  applctn_quick_poll(0) ;
        if (ReadSensorValue (device_r, device_w, SEC_READD2, &d2) < 0)
    return -1 ; // Failure
        DT = d2 - ((8.0 * cal[4]) + 20224.0);
        OFF = cal[1] * 4.0 + (((cal[3] - 512.0) * DT) / 4096.0);
        SENSE = 24576.0 + cal[0] + ((cal[2] * DT) / 1024.0);
        X = ((SENSE * (d1 - 7168.0)) / 16384.0) - OFF;
        Press = 250.0 + X / 32.0;

        *Pressure = (float) Press;

        T = 20.0 + ((DT * (cal[5] + 50.0)) / 10240.0);

        *Temperature = (float) T;

        return 0 ; // Ok
}



static int
PressureReset (int device_r, int device_w)
{
        //~ if (!OpenPIOS_A() || !ConfigWrite())
        //~ {
        //~ werr(0, "PressureReset failed") ;
        //~ return 0 ;
        //~ }

        WritePressureSensor (device_r, device_w, SEC_RESET);

        return 1;
}


// Open (i.e. o/p goes high) the channel A switch
static int
OpenA (int device)
{
        int I = 0XFF, res = 0;

        int Ind = 5;
        do
        {
                I = ReadSwitch12 (device, FALSE);
                //res = I | 0X20;
                WriteSwitch12 (device, I | 0X20);
                I = ReadSwitch12 (device, FALSE);
                Ind--;
                res = (I & 0XDF);
                if (res != 0)
                        return TRUE;
        }
        while (Ind > 0);	//&& ((I & 0XDF)!=0));
        //while ((Ind>0) && ((I & 0XDF)!=0));
        return (Ind > 0);
}

// Open (i.e. o/p goes high) the channel B switch
static int
OpenB (int device)
{
        int I = 0XFF, res = 0;

        int Ind = 5;
        do
        {
                I = ReadSwitch12(device, FALSE);
                //res = I | 0X40;
                WriteSwitch12 (device, I | 0X40);
                I = ReadSwitch12 (device, FALSE);
                Ind--;
                res = (I & 0XBF);
                if (res != 0)
                        return TRUE;
        }
        while (Ind > 0);	//&& ((I & 0XDF)!=0));
        //while ((Ind>0) && ((I & 0XDF)!=0));
        return (Ind > 0);
}

// Open B (data in/out) on both reader and writer
static int
OpenPIOS_B (int device_r, int device_w)
{
        //int Pio_B=1;
        //return OpenPIOS(portnum, Pio_B);
        if (!OpenB (device_r))
                return FALSE;
        if (!OpenB (device_w))
                return FALSE;

        return TRUE;
}

// Open A (clock) on both reader and writer
static int
OpenPIOS_A (int device_r, int device_w)
{
        //int Pio_A=0;
        //return OpenPIOS(portnum, Pio_A);
        if (!OpenA (device_r))
                return FALSE;
        if (!OpenA (device_w))
                return FALSE;

        return TRUE;
}

// Send a Channel Access command sequence
static int
ChannelAccess (int device, int AChanC1, int AChanC2,
              int * ACInfo)
{
        if (0 == devices_access (device))
  {
    werr(WERR_WARNING+WERR_AUTOCLOSE, "ChannelAccess - devices_access(%s) failed",
      devices_list[device].menu_entry) ;
                return 0;
  }

        owTouchByte (devices_portnum(device), 0XF5);
        owTouchByte (devices_portnum(device), AChanC1);
        owTouchByte (devices_portnum(device), AChanC2);
        *ACInfo = owTouchByte (devices_portnum(device), 0XFF);

        return TRUE ;
}

//---------------------------------------------------------------------------
static int
ConfigWrite (int device_w)
{
        int ACInfo = 0;
        return ChannelAccess (device_w, CFG_WRITE, 0xFF,
                              &ACInfo);

}

//---------------------------------------------------------------------------
static int
ConfigRead (int device_r)
{
        int ACInfo = 0;
        return ChannelAccess (device_r, CFG_READ, 0xFF,
                              &ACInfo);

}

//---------------------------------------------------------------------------
static int
ConfigReadPulse (int device_r)
{
        int ACInfo = 0;
        return ChannelAccess (device_r, CFG_RPULSE, 0xFF,
                              &ACInfo);

}



static int
WritePressureSensor (int device_r, int device_w, char *Command)
{
        int Size, i;
        int p ;
        Size = strlen (Command);

        //printf ("WritePressureSensor(\"%s\")\n", Command);

        p = devices_portnum(device_r) ;

        if (ConfigWrite (device_w))
        {
    applctn_quick_poll(0) ;
                for (i = 0; i < Size; ++i)
                {
                        if (Command[i] == '0')
                                SendPressureBit (p, 0);
                        else
                                SendPressureBit (p, 1);
                }
                SendPressureBit (p, 0);
                return 1;
        }
        return 0;
}


static void
ReadPressureSensor (int device_r, int CountBit, char buffer[], int buflen)
{
        int i, p ;

        assert (CountBit + 1 >= buflen);

        p = devices_portnum(device_r) ;

        buffer[0] = '\0';

        if (!ConfigRead (device_r))
                return;

  applctn_quick_poll(0) ;

        for (i = 0; i < CountBit; i++)
        {
                if (ReadPressureBit (p))
                        buffer[i] = '1';
                else
                        buffer[i] = '0';
        }

        buffer[i] = '\0';
        //printf ("ReadPressureSensor(%d): %s\n", CountBit, buffer);

        return;
}



static int
SendPressureBit (int portnum, int value)
{
        //printf ("SendPressureBit-> %d\n", value);
        if (value > 0)
        {
                /*owTouchBit (0);
                owTouchBit (1);
                owTouchBit (1);
                owTouchBit (1);
                owTouchBit (0);
                owTouchBit (0);*/
                /*int sendbits[] = {0, 1, 1, 1, 0, 0, 0, 0} ;
                * owTouchBits(sendbits, 8) ; */

    owWriteByte(portnum, 0x0e) ;
        }
        else
        {
                /*owTouchBit (0);
                owTouchBit (0);
                owTouchBit (1);
                owTouchBit (0);
                owTouchBit (0);
                owTouchBit (0);*/
                /*int sendbits[] = {0, 0, 1, 0, 0, 0, 0, 0} ;
                * owTouchBits(sendbits, 8) ; */

    owWriteByte(portnum, 0x04) ;
        }

        return 0;
}

//
// Function SendPressureBit(Value : Byte): Byte;
// Begin
//    if Valor > 0  Then
//     Begin
//      TMBit(0);              // Dallas API Function
//      TMBit(1);
//      TMBit(1);
//      TMBit(1);
//      TMBit(0);
//      TMBit(0);
//     End
//    Else
//     Begin
//      TMBit(0);
//      TMBit(0);
//      TMBit(1);
//      TMBit(0);
//      TMBit(0);
//      TMBit(0);
//     End;
//   Result = 0;
// End;
//
// /******************************************************/
//

static int
ReadPressureBit (int portnum)
{
        int /*crc = 0, */ res = 0;

  res = owReadByte(portnum) ;
  //printf("0x%x\n", res) ;
  res &= 0x80 ;
  owWriteByte(portnum, 0xfa) ;

        /*owTouchBit (1);
        owTouchBit (1);
        owTouchBit (1);
        owTouchBit (1);
        owTouchBit (1);
        owTouchBit (1);
        owTouchBit (1);

        res = owTouchBit (1);

        owTouchBit (0);
        owTouchBit (1);
        owTouchBit (0);
        owTouchBit (1);
        owTouchBit (1);
        owTouchBit (1);
        owTouchBit (1);
        owTouchBit (1);*/

        /*int sendbits[] = {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1} ;
        *
        * owTouchBits(sendbits, 16) ;
        * res = sendbits[7] ; */

        //printf ("ReadPressureBit-> %d\n", res);

        return (res != 0) ;
}
