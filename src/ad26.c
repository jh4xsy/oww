/* ad26.c */

/* For Oww project
   Tue 30th January 2001
   This is a hacked version of ad26.c by Dallas Semiconductor
   Wound back owXXXX function names to older MLanXXXX versions.
   Added werr.c error reporting
   Added applctn.c calls to keep the GUI running
*/

//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
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
//--------------------------------------------------------------------------

// Include Files
#include <stdio.h>
#include <math.h>
#include "werr.h"
//#include "mlan.h"
#include "ownet.h"
#include "applctn.h"
#include "devices.h"
#include "ad26.h"
#include "setup.h"
#include "intl.h"


#define RETRY_DELAY 125 /* ms */

/* external MLan functions */
/*extern int  MLanBlock(int, uchar *, int);
extern int  MLanReadByte(void);
extern int  MLanWriteByte(int sendbyte);
extern void MLanSerialNum(uchar *, int);
extern int  MLanAccess(void);
extern int  MLanLevel(int);
extern void msDelay(int);*/
//extern uchar dowcrc(uchar);



extern int64_t csGettick(void) ;


//extern uchar DOWCRC;

static void recharge(int portnum)
{
  if (setup_recharge != 0)
  {
    owLevel(portnum, MODE_STRONG5);
    msDelay(RETRY_DELAY) ;
    owLevel(portnum, MODE_NORMAL); 
  }
  else
  {
    msDelay(RETRY_DELAY) ;
  }
}

/* Configure a DS2438 for reading Vsens (current) */

int ad26_conf_current(int adc)
{
   uchar send_block[50];
   int send_cnt=0;
   int i;
   int busybyte;
   ushort lastcrc8=0;

   devices_access(adc) ;

   // Recall the Status/Configuration page
   // Recall command
   send_block[send_cnt++] = 0xB8;

   // Page to Recall
   send_block[send_cnt++] = 0x00;

   if(!owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
      return FALSE;

   send_cnt = 0;

   if (devices_access(adc))
   {
      // Read the Status/Configuration byte
      // Read scratchpad command
      send_block[send_cnt++] = 0xBE;

      // Page for the Status/Configuration byte
      send_block[send_cnt++] = 0x00;

      for(i=0;i<9;i++)
         send_block[send_cnt++] = 0xFF;

      if(owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
      {
        setcrc8(devices_portnum(adc),0);
        //DOWCRC = 0;

        /* perform the CRC8 on the last 8 bytes of packet */

         for(i=2;i<send_cnt;i++)
            lastcrc8 = docrc8(devices_portnum(adc),send_block[i]);

         if(lastcrc8 != 0x00)
            return FALSE;
      }//Block
      else
         return FALSE;

      if (send_block[2] & 0x01) 
        return TRUE;
   }//Access

   if (devices_access(adc))
   {
      send_cnt = 0;
      // Write the Status/Configuration byte
      // Write scratchpad command
      send_block[send_cnt++] = 0x4E;

      // Write page
      send_block[send_cnt++] = 0x00;

      send_block[send_cnt++] = send_block[2] | 0x01;

      for(i=0;i<7;i++)
         send_block[send_cnt++] = send_block[i+4];

      if(owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
      {
         send_cnt = 0;

         if (devices_access(adc))
         {
            // Copy the Status/Configuration byte
            // Copy scratchpad command
            send_block[send_cnt++] = 0x48;

            // Copy page
            send_block[send_cnt++] = 0x00;

            if(owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
            {
              int64_t tick ;

              tick = csGettick() ;
               busybyte = owReadByte(devices_portnum(adc));

               while(busybyte == 0)
               {
                 int64_t tt = csGettick() - tick ;

                 if (tt > 100LL)
                 {
                   werr(WERR_WARNING+WERR_AUTOCLOSE,
                        _("1-s timeout reading voltage")) ;
                   return FALSE ;
                 }
                 if (tt > 10)
                 {
                   if (tt < 20) msDelay(10) ; /* Idle polling */
                   else applctn_quick_poll(0) ; /* Agressive polling */
                 }
                 busybyte = owReadByte(devices_portnum(adc));
               }

               return TRUE;
            }//Block
         }//Access
      }//Block

   }//Access

   return FALSE;
}

int Volt_AD(int vdd, /*uchar *SNum*/ int adc)
{
   uchar send_block[50];
   uchar test;
   int send_cnt=0;
   int i;
   ushort lastcrc8=0;
   int busybyte;

   devices_access(adc) ;

   // Recall the Status/Configuration page
   // Recall command
   send_block[send_cnt++] = 0xB8;

   // Page to Recall
   send_block[send_cnt++] = 0x00;

   if(!owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
      return FALSE;

   send_cnt = 0;

   if (devices_access(adc))
   {
      // Read the Status/Configuration byte
      // Read scratchpad command
      send_block[send_cnt++] = 0xBE;

      // Page for the Status/Configuration byte
      send_block[send_cnt++] = 0x00;

      for(i=0;i<9;i++)
         send_block[send_cnt++] = 0xFF;

      if(owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
      {
         setcrc8(devices_portnum(adc),0);
        //DOWCRC = 0;

        /* perform the CRC8 on the last 8 bytes of packet */

         for(i=2;i<send_cnt;i++)
            lastcrc8 = docrc8(devices_portnum(adc),send_block[i]);

         if(lastcrc8 != 0x00)
            return FALSE;
      }//Block
      else
         return FALSE;

      test = send_block[2] & 0x08;
      if(((test == 0x08) && vdd) || ((test == 0x00) && !(vdd)))
         return TRUE;
   }//Access

   if (devices_access(adc))
   {
      send_cnt = 0;
      // Write the Status/Configuration byte
      // Write scratchpad command
      send_block[send_cnt++] = 0x4E;

      // Write page
      send_block[send_cnt++] = 0x00;

      if(vdd)
         send_block[send_cnt++] = send_block[2] | 0x08;
      else
         send_block[send_cnt++] = send_block[2] & 0xF7;

      for(i=0;i<7;i++)
         send_block[send_cnt++] = send_block[i+4];

      if(owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
      {
         send_cnt = 0;

         if (devices_access(adc))
         {
            // Copy the Status/Configuration byte
            // Copy scratchpad command
            send_block[send_cnt++] = 0x48;

            // Copy page
            send_block[send_cnt++] = 0x00;

            if(owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
            {
              int64_t tick ;

              tick = csGettick() ;
               busybyte = owReadByte(devices_portnum(adc));

               while(busybyte == 0)
               {
                 int64_t tt = csGettick() - tick ;

                 if (tt > 100LL)
                 {
                   werr(WERR_WARNING+WERR_AUTOCLOSE,
                        _("1-s timeout reading voltage")) ;
                   return FALSE ;
                 }
                 if (tt > 10)
                 {
                   if (tt < 20) msDelay(10) ; /* Idle polling */
                   else applctn_quick_poll(0) ; /* Agressive polling */
                 }
                 busybyte = owReadByte(devices_portnum(adc));
               }

               return TRUE;
            }//Block
         }//Access
      }//Block

   }//Access

   return FALSE;
}

/* ad26_current_reading will call itself recursively to maximum "depth"
   in the event of bus resets or other problems */

int ad26_current_reading(int adc, int depth, float *result)
{
   uchar send_block[50];
   int send_cnt=0;
   int i;
   //int busybyte;
   short current ;
   ushort lastcrc8=0;
   //unsigned long tick ;

  /* Return error condition if we have reached 0 depth */

  if (depth <= 0)
  {
    werr(
      WERR_WARNING,
      _("Too many retries reading %s current. Giving up."),
      devices_list[adc].menu_entry) ;
    return -1 ;
  }

   if(ad26_conf_current(adc))
   {
      if (devices_access(adc))
      {
         if(!owWriteByte(devices_portnum(adc),0xB4))
         {
           werr(WERR_WARNING,
             _("ad26_current_reading didn't write correctly, reading %s"),
             devices_list[adc].menu_entry) ;
           //recharge() ;
           return ad26_current_reading(adc, depth-1, result) ;
         }
      }

      if (devices_access(adc))
      {
         // Recall the Status/Configuration page
         // Recall command
         send_block[send_cnt++] = 0xB8;

         // Page to Recall
         send_block[send_cnt++] = 0x00;

         if(!owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
            return -1 ;
      }
      else
      {
        werr(WERR_DEBUG0,
          "access failure, reading %s",
          devices_list[adc].menu_entry) ;
        //recharge() ;
        return ad26_current_reading(adc, depth-1, result) ;
      }

      send_cnt = 0;

      if (devices_access(adc))
      {
         // Read the Status/Configuration byte
         // Read scratchpad command
         send_block[send_cnt++] = 0xBE;

         // Page for the Status/Configuration byte
         send_block[send_cnt++] = 0x00;

         for(i=0;i<9;i++)
            send_block[send_cnt++] = 0xFF;

         if(owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
         {
           setcrc8(devices_portnum(adc),0);
            //DOWCRC = 0;

           for(i=2;i<send_cnt;i++)
              lastcrc8 = docrc8(devices_portnum(adc),send_block[i]);
  
           if(lastcrc8 != 0x00)
              return -1 ;
         }
         else
         {
           werr(WERR_DEBUG0,
             "Block not sent, reading %s",
             devices_list[adc].menu_entry) ;
           //recharge() ;
           return ad26_current_reading(adc, depth-1, result) ;
         }

         current = (send_block[8] << 8) | send_block[7];
         if (send_block[8] & 0x80) // Check sign bit
           current |= ~0x3ff ; // Set all bits above bit 9
         *result = (float) current;
      }//Access
      else
      {
        werr(WERR_WARNING,
          _("Access failed, reading %s"),
          devices_list[adc].menu_entry) ;
        //recharge() ;
        return ad26_current_reading(adc, depth-1, result) ;
      }
   }
   else
   {
     werr(WERR_WARNING,
       _("ad26_current_reading failed, reading %s"),
       devices_list[adc].menu_entry) ;
     //recharge() ;
     return ad26_current_reading(adc, depth-1, result) ;
   }

   return 0 ; /* Ok */
}
/* Volt_Reading will call itself recursively to maximum "depth"
   in the event of bus resets or other problems */

#define FAILED -1.0F

float Volt_Reading(int vdd, int adc, int depth)
{
   uchar send_block[50];
   int send_cnt=0;
   int i;
   int busybyte;
   ushort volts;
   float ret= FAILED ;
   int64_t tick ;
   ushort lastcrc8=0;

  /* Return error condition if we have reached 0 depth */

  if (depth <= 0)
  {
    werr(
      WERR_WARNING,
      _("Too many retries reading %s. Giving up."),
      devices_list[adc].menu_entry) ;
    return FAILED ;
  }

   if(Volt_AD(vdd, adc))
   {
      if (devices_access(adc))
      {
         if(!owWriteByte(devices_portnum(adc),0xB4))
         {
           werr(WERR_WARNING,
             _("Volt_AD didn't write correctly, reading %s"),
             devices_list[adc].menu_entry) ;
           recharge(devices_portnum(adc)) ;
           return Volt_Reading(vdd, adc, depth-1) ;
         }

         tick = csGettick() ;
         busybyte = owReadByte(devices_portnum(adc));

         while(busybyte == 0)
         {
           long tt = csGettick() - tick ;

           if (tt > 100)
           {
             werr(WERR_DEBUG0,
               "1-s timeout reading voltage, %s",
               devices_list[adc].menu_entry) ;
             return Volt_Reading(vdd, adc, depth-1) ;
           }
           if (tt > 10)
           {
             if (tt < 20) msDelay(10) ; /* Idle polling */
             else applctn_quick_poll(0) ; /* Agressive polling */
           }
           busybyte = owReadByte(devices_portnum(adc));
         }
      }

      if (devices_access(adc))
      {
         // Recall the Status/Configuration page
         // Recall command
         send_block[send_cnt++] = 0xB8;

         // Page to Recall
         send_block[send_cnt++] = 0x00;

         if(!owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
            return FAILED ;
      }
      else
      {
        werr(WERR_DEBUG0,
          "access failure, reading %s",
          devices_list[adc].menu_entry) ;
        recharge(devices_portnum(adc)) ;
        return Volt_Reading(vdd, adc, depth-1) ;
      }

      send_cnt = 0;

      /*if(owAccess())*/
      if (devices_access(adc))
      {
         // Read the Status/Configuration byte
         // Read scratchpad command
         send_block[send_cnt++] = 0xBE;

         // Page for the Status/Configuration byte
         send_block[send_cnt++] = 0x00;

         for(i=0;i<9;i++)
            send_block[send_cnt++] = 0xFF;

         if(owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
         {
            //DOWCRC = 0;
           setcrc8(devices_portnum(adc),0);
	   /*setcrc8(portnum,0);*/

            for(i=2;i<send_cnt;i++)
              lastcrc8 = docrc8(devices_portnum(adc),send_block[i]);              
	      //dowcrc(send_block[i]);
               /*lastcrc8 = docrc8(portnum,send_block[i]);*/

            if(lastcrc8 != 0x00)
               return ret;

         }
         else
         {
           werr(WERR_DEBUG0,
             "Block not sent, reading %s",
             devices_list[adc].menu_entry) ;
           recharge(devices_portnum(adc)) ;
           return Volt_Reading(vdd, adc, depth-1) ;
         }

         /* As per Dallas recomendation, check that we just read
            the correct ADC, in case the 2438 reset itself. */

         if ((send_block[2] & 0x08) && !vdd)
         {
           werr(WERR_DEBUG0,
             "DS2438 reset itself, reading %s",
             devices_list[adc].menu_entry) ;
           recharge(devices_portnum(adc)) ;
           if (depth>1)
             return Volt_Reading(vdd, adc, depth-1) ;
         }

         volts = (send_block[6] << 8) | send_block[5];
         ret = (float) volts/100;
      }//Access
      else
      {
        werr(WERR_WARNING,
          _("Access failed, reading %s"),
          devices_list[adc].menu_entry) ;
        recharge(devices_portnum(adc)) ;
        return Volt_Reading(vdd, adc, depth-1) ;
      }
   }
   else
   {
     werr(WERR_WARNING,
       _("Volt_AD failed, reading %s"),
       devices_list[adc].menu_entry) ;
     recharge(devices_portnum(adc)) ;
     return Volt_Reading(vdd, adc, depth-1) ;
   }

   return ret ;
}
#undef FAILED

int ad26_start_tempconv(int adc)
{
   int busybyte ;
   int64_t tick ;

   if(devices_access(adc))
      // Convert Temperature command
      owWriteByte(devices_portnum(adc),0x44);

   busybyte = owReadByte(devices_portnum(adc));
   tick = csGettick() ;

   while(busybyte == 0)
   {
     long tt = csGettick() - tick ;

     if (tt > 100)
     {
       werr(WERR_WARNING+WERR_AUTOCLOSE,
            _("1-s timeout converting temperature") );
       return FALSE ;
     }
     if (tt > 10)
     {
       if (tt < 20) msDelay(10) ; /* Idle polling */
       else applctn_quick_poll(0) ; /* Agressive polling */
     }
     busybyte = owReadByte(devices_portnum(adc));
   }

   return 0 ;
}

/**
 * Reads the temperature from the DS2438.
 *
 * @param adc  The device number for the DS2438
 * @param T    Pointer to a float variable to hold the result
 *
 * @return 0 on success, -1 on error
 */
int Get_Temperature(int adc, float *T)
{
   //double ret=-1.0;
   short rs ;
   uchar send_block[50];
   int send_cnt=0;
   int i ;
   ushort lastcrc8=0;

   /* N.B. This is not issuing a copy scratchpad command.
           It relies on Volt_AD having done this already. */

   /*owSerialNum(SNum,FALSE);*/

  *T = 85.0F ;

   if(devices_access(adc) /*owAccess()*/)
   {
      // Read the Status/Configuration byte
      // Read scratchpad command
      send_block[send_cnt++] = 0xBE;

      // Page for the Status/Configuration byte
      send_block[send_cnt++] = 0x00;

      for(i=0;i<9;i++)
         send_block[send_cnt++] = 0xFF;

      if(owBlock(devices_portnum(adc),FALSE,send_block,send_cnt))
      {
         setcrc8(devices_portnum(adc),0);

         for(i=2;i<send_cnt;i++)
            lastcrc8 = docrc8(devices_portnum(adc),send_block[i]);

         if(lastcrc8 != 0x00)
            return -1 ;

      }
      else
         return -1 ;

      rs = ((send_block[4] << 8) | send_block[3]) >> 3 ;
      if (rs & 0x1000) rs |= ~0xFFF ;

      *T = (float) rs * 0.03125F ;

      #ifdef HAVE_FINITEF
      if (!finitef(*T) && werr_will_output(WERR_DEBUG0))
      {
        werr(WERR_DEBUG0, "nan detected for Get_Temperature(%d,)", adc) ;
        werr(WERR_DEBUG0, "send_block[3] = %d, [4] = %d",
          send_block[3], send_block[4]) ;
        werr(WERR_DEBUG0, "rs = %d", rs) ;
        werr(WERR_DEBUG0, "*T = %f", *T) ;
      }
      #endif // HAVE_FINITEF
      return 0 ; /* Ok */
   }//Access

   return -1 ;
}
