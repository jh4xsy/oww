//---------------------------------------------------------------------------
// Copyright (C) 2003 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//---------------------------------------------------------------------------
//
//  multilnk.C - Wrapper to hook all adapter types in the 1-Wire Public 
//               Domain API for link functions.
//
//  Version: 3.00
//

// Version for libowpd by Dr. Simon J. Melhuish
// simon@melhuish.info

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#ifdef WIN32
#include "config-w32gcc.h"
#else
#include "../config.h"
#endif
#endif
//#include "werr.h"
#include "ownet.h"
#include <time.h>
#include <sys/time.h>

#include <unistd.h>

#include "applctn.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifndef HAVE_INT64_T
typedef long long int		int64_t;
#endif


// If TRUE, puts a delay in owTouchReset to compensate for alarming clocks.
SMALLINT FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = FALSE; // default owTouchReset to quickest response. 

extern SMALLINT default_type; 
#ifdef HAVE_USB     
extern SMALLINT SFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE;
#endif
extern SMALLINT UFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE;

//--------------------------------------------------------------------------
// Reset all of the devices on the 1-Wire Net and return the result.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Returns: TRUE(1):  presense pulse(s) detected, device(s) reset
//          FALSE(0): no presense pulses detected
//
SMALLINT owTouchReset(int portnum)
{
   // check global flag does not match specific adapters
   if (FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE != 
       UFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE)
   {
      UFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = 
         FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE;
#ifdef HAVE_USB     
      SFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = 
         FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE;
#endif
   }

   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB     
      case DS9490:  
        return owTouchReset_DS9490(portnum & 0xFF);
#endif      
      case DS9097U: 
      default: 
       return owTouchReset_DS9097U(portnum & 0xFF);
   };
}


//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and return the
// result 1 bit read from the 1-Wire Net.  The parameter 'sendbit'
// least significant bit is used and the least significant bit
// of the result is the return bit.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'sendbit'     - the least significant bit is the bit to send
//
// Returns: 0:   0 bit read from sendbit
//          1:   1 bit read from sendbit
//
SMALLINT owTouchBit(int portnum, SMALLINT sendbit)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB     
      case DS9490:  
        return owTouchBit_DS9490(portnum & 0xFF, sendbit);
#endif
     case DS9097U: 
     default: 
        return owTouchBit_DS9097U(portnum & 0xFF, sendbit);
   };
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).
// The parameter 'sendbyte' least significant 8 bits are used.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'sendbyte'   - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same
//
SMALLINT owTouchByte(int portnum, SMALLINT sendbyte)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB     
      case DS9490:  
        return owTouchByte_DS9490(portnum & 0xFF, sendbyte);
#endif
     case DS9097U: 
     default: 
        return owTouchByte_DS9097U(portnum & 0xFF, sendbyte);
   };
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the MicroLAN and verify that the
// 8 bits read from the MicroLAN is the same (write operation).
// The parameter 'sendbyte' least significant 8 bits are used.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'sendbyte'   - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same
//
SMALLINT owWriteByte(int portnum, SMALLINT sendbyte)
{
   return (owTouchByte(portnum,sendbyte) == sendbyte) ? TRUE : FALSE;
}

//--------------------------------------------------------------------------
// Send 8 bits of read communication to the 1-Wire Net and and return the
// result 8 bits read from the 1-Wire Net.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Returns:  TRUE:  8 bytes read from 1-Wire Net
//           FALSE: the 8 bytes were not read
//
SMALLINT owReadByte(int portnum)
{
   return owTouchByte(portnum,0xFF);
}

//--------------------------------------------------------------------------
// Set the 1-Wire Net communucation speed.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'new_speed'  - new speed defined as
//                MODE_NORMAL     0x00
//                MODE_OVERDRIVE  0x01
//
// Returns:  current 1-Wire Net speed
//
SMALLINT owSpeed(int portnum, SMALLINT new_speed)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB     
      case DS9490:  
        return owSpeed_DS9490(portnum & 0xFF, new_speed);
#endif
      case DS9097U: 
      default: 
       return owSpeed_DS9097U(portnum & 0xFF, new_speed);
   };
}

//--------------------------------------------------------------------------
// Set the 1-Wire Net line level.  The values for new_level are
// as follows:
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number is provided to
//               indicate the symbolic port number.
// 'new_level' - new level defined as
//                MODE_NORMAL     0x00
//                MODE_STRONG5    0x02
//                MODE_PROGRAM    0x04
//                MODE_BREAK      0x08 (not supported)
//
// Returns:  current 1-Wire Net level
//
SMALLINT owLevel(int portnum, SMALLINT new_level)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB     
      case DS9490:  
        return owLevel_DS9490(portnum & 0xFF, new_level);
#endif      
     case DS9097U: 
     default: 
        return owLevel_DS9097U(portnum & 0xFF, new_level);
   };
}

//--------------------------------------------------------------------------
// This procedure creates a fixed 480 microseconds 12 volt pulse
// on the 1-Wire Net for programming EPROM iButtons.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Returns:  TRUE  successful
//           FALSE program voltage not available
//
SMALLINT owProgramPulse(int portnum)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB     
      case DS9490:  
        return owProgramPulse_DS9490(portnum & 0xFF);
#endif
     case DS9097U: 
     default: 
        return owProgramPulse_DS9097U(portnum & 0xFF);
   };
}

//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
//
void msDelay(int len)
{
  struct timeval tv, now_tv, subres ;
  unsigned long int ulen ;

  /* For short delays, just sleep */
  if (len <= 10)
  {
    usleep(1000UL * (unsigned long) len) ;
    return ;
  }
   
  /* For longer delays we'll do some polling whilst we wait */
  /* Get start time */
  gettimeofday(&tv, NULL) ;

  /* len converted to micro seconds */
  ulen = 1000UL * len ;
  
  /* Loop whilst waiting */
  do
  {
    /* Quick poll */
    applctn_quick_poll(0) ;
    
    /* Wait for 10 ms */
    usleep(10000UL) ;
    
    /* Check time now */
    gettimeofday(&now_tv, NULL) ;
    timersub(&now_tv,&tv,&subres);
  } while
      ((subres.tv_sec*1000000UL+subres.tv_usec) < ulen) ;
}

//struct timeval *getTick(struct timeval *subres)
//{
//   struct timeval  nowtv;
//   static struct timeval starttv = {0,0};
//
//   if (!timerisset(&starttv))
//	 gettimeofday(&starttv, NULL);
//
//   gettimeofday(&nowtv,NULL);
//
//   timersub(&nowtv, &starttv, subres);
//   return &subres;
//}

//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
// This implementation actually returns ms since epoch, like in Java
int64_t msGettick(void)
{
  struct timeval  nowtv;
//  static long prev = 0L;
//  long now;

  gettimeofday(&nowtv,NULL);

  return (int64_t) nowtv.tv_sec * 1000LL + ((int64_t) nowtv.tv_usec / 1000LL);

//  if (prev != 0L)
//	if (prev > now)
//	{
//	  werr(WERR_DEBUG0, "ms tick went backwards (%12li -> %12li)", prev, now);
//	  werr(WERR_DEBUG0, "now.tv_sec = %12li, now.tv_usec = %12li", nowtv.tv_sec, nowtv.tv_usec);
//	  werr(WERR_DEBUG0, "now.tv_sec = %12lx, now.tv_usec = %12lx", nowtv.tv_sec, nowtv.tv_usec);
//	}

//  prev = now;

//  return ;
}

//--------------------------------------------------------------------------
// Get the current centisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
// This implementation actually returns cs since epoch
int64_t csGettick(void)
{
  struct timeval  nowtv;

  gettimeofday(&nowtv,NULL);

  return (int64_t) nowtv.tv_sec * 100LL + ((int64_t) nowtv.tv_usec / 10000LL);
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).  
// The parameter 'sendbyte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same 
//
SMALLINT owWriteBytePower(int portnum, SMALLINT sendbyte)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB
      case DS9490:  
        return owWriteBytePower_DS9490(portnum & 0xFF, sendbyte);
#endif
     case DS9097U: 
     default: 
       return owWriteBytePower_DS9097U(portnum & 0xFF, sendbyte);
   };
}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and verify that the
// response matches the 'applyPowerResponse' bit and apply power delivery
// to the 1-Wire net.  Note that some implementations may apply the power
// first and then turn it off if the response is incorrect.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'applyPowerResponse' - 1 bit response to check, if correct then start
//                        power delivery 
//
// Returns:  TRUE: bit written and response correct, strong pullup now on
//           FALSE: response incorrect
//
SMALLINT owReadBitPower(int portnum, SMALLINT applyPowerResponse)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB
      case DS9490:  
        return owReadBitPower_DS9490(portnum & 0xFF, applyPowerResponse);
#endif
     case DS9097U: 
     default: 
       return owReadBitPower_DS9097U(portnum & 0xFF, applyPowerResponse);
   };
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).  
// The parameter 'sendbyte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant bit)
//
// Returns:  TRUE: bytes written and echo was the same, strong pullup now on
//           FALSE: echo was not the same 
//
SMALLINT owReadBytePower(int portnum)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB
      case DS9490:  return owReadBytePower_DS9490(portnum & 0xFF);
#endif
      case DS9097U: 
      default: 
       return owReadBytePower_DS9097U(portnum & 0xFF);
   };
}

//--------------------------------------------------------------------------
// This procedure indicates wether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  because all userial adapters have over drive. 
//
SMALLINT owHasPowerDelivery(int portnum)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB
      case DS9490:  
        return owHasPowerDelivery_DS9490(portnum & 0xFF);
#endif
     case DS9097U: 
     default: 
       return owHasPowerDelivery_DS9097U(portnum & 0xFF);
   };
}

//--------------------------------------------------------------------------
// This procedure indicates wether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  because all userial adapters have over drive. 
//
SMALLINT owHasOverDrive(int portnum)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB
      case DS9490:  return owHasOverDrive_DS9490(portnum & 0xFF);
#endif
     case DS9097U: 
     default: 
       return owHasOverDrive_DS9097U(portnum & 0xFF);
   };
}

//--------------------------------------------------------------------------
// This procedure creates a fixed 480 microseconds 12 volt pulse 
// on the 1-Wire Net for programming EPROM iButtons.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  program volatage available
//           FALSE program voltage not available  
SMALLINT owHasProgramPulse(int portnum)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB
      case DS9490:  
        return owHasProgramPulse_DS9490(portnum & 0xFF);
#endif
     case DS9097U: 
     default: 
        return owHasProgramPulse_DS9097U(portnum & 0xFF);
   };
}
