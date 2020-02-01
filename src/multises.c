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
//  multises.c - Wrapper to hook all adapter types in the 1-Wire Public
//               Domain API for session functions.
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

#include <string.h>
#include "ownet.h"

// SJM - DS1410E not supported in this version

SMALLINT default_type = 0;

//---------------------------------------------------------------------------
// Attempt to acquire the specified 1-Wire net.
//
// 'port_zstr'     - zero terminated port name.  For this platform
//                   use format {port number, port type}.  The port types
//                   are: 2 LPT/DS1410E, 5 COM/DS2480B, 6 USB/DS9490
//
// Returns: port number or -1 if not successful in setting up the port.
//
SMALLINT owAcquireEx(char *port_zstr)
{
   SMALLINT rt = -1;
   char tmp_port_zstr[16];
   int type, num;

   if (port_zstr)
   {
      if (sscanf(port_zstr,"{%d,%d}",&num,&type) == 2)
      {
         switch (type)
         {
#ifdef HAVE_USB            
           case DS9490:
               sprintf(tmp_port_zstr,"USB:%d",num);
               rt = owAcquireEx_DS9490(tmp_port_zstr);
	       type = DS9490;
               break;
#endif
            case DS9097U:
               sprintf(tmp_port_zstr,"/dev/ttyS%d",num);
               rt = owAcquireEx_DS9097U(tmp_port_zstr);
               type = DS9097U;
               break;
            default:
               rt = -1;
               break;
         }
      }
#ifdef HAVE_USB            
      else if (strncmp(port_zstr, "USB", 3) == 0)
      {
         rt = owAcquireEx_DS9490(port_zstr);
	 type = DS9490;
      }
#endif
      else
      {
         rt = owAcquireEx_DS9097U(port_zstr);
	 type = DS9097U;
      }
      /*else if ( (sscanf(port_zstr,"%d",&num) == 1) &&
                (num>=1 && num <=3) )
         rt = owAcquire_DS1410E(num, port_zstr);*/
   }

   if (rt >=0)
      return ((type << 8) | rt);
   else
      return -1;
}

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'port_zstr'  - zero terminated port name.  Format indicates the adapter
//                type.
//
// Returns: TRUE - success, port opened
//
SMALLINT owAcquire(int portnum, char *port_zstr)
{
   char tmp_port_zstr[16];
   int type, num;

   // legacy call, attempt to discern adapter type from string format
   // if adapter type is not embedded in portnum
   if ((portnum & 0xFF00) == 0 && port_zstr)
   {
      if (sscanf(port_zstr,"{%d,%d}",&num,&type) == 2)
      {
         switch (type)
         {
#ifdef HAVE_USB            
           case DS9490:
               default_type = DS9490;
               //sprintf(tmp_port_zstr,"USB:%d",num);
               return owAcquire_DS9490(num, "USB");
               break;
#endif
            default:
            case DS9097U:
               default_type = DS9097U;
               sprintf(tmp_port_zstr,"/dev/ttyS%d",num);
               return owAcquire_DS9097U(num, tmp_port_zstr);
               break;
         }
      }
#ifdef HAVE_USB            
      else if (strncmp(port_zstr, "USB", 3) == 0)
      {
         default_type = DS9490;
	 return owAcquire_DS9490(portnum, port_zstr);
      }
#endif
      
      default_type = DS9097U;
      return owAcquire_DS9097U(portnum, port_zstr);
   }

   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB            
     case DS9490:
         return owAcquire_DS9490(portnum & 0xFF, port_zstr);
#endif
      default:
      case DS9097U:
         return owAcquire_DS9097U(portnum & 0xFF, port_zstr);
   }
}

//---------------------------------------------------------------------------
// Release the previously acquired a 1-Wire net.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
//
void owRelease(int portnum)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
#ifdef HAVE_USB            
     case DS9490:
         owRelease_DS9490(portnum & 0xFF);
         break;
#endif
      default:
      case DS9097U: 
         owRelease_DS9097U(portnum & 0xFF);
         break;
   };
}
