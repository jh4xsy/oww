//
// C Implementation: thermometer
//
// Description: 
//
//
// Author: Simon Melhuish <simon@melhuish.info>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <assert.h>

#include "wstypes.h"
#include "thermometer.h"
#include "devices.h"
#include "werr.h"
#include "ownet.h"
#include "progstate.h"
#include "ad26.h"

#define VOLT_READING_TRIES 6

extern wsstruct ws;

/*static void
thermometer_init(void)
{
  int i ;
  
  for (i=0; i<MAXTEMPS; ++i)
  {
    thermometer *T ;
    
    T = (thermometer *) devices_list[devices_T1 + i].local ;
    T->state = thermometer_init ;
  }
}*/

static int
thermometer_read_ds2438_thermometer (void)
{
  float Vad, /*Vdd, */ T;

  /* Start the temperature conversion */
  if (ad26_start_tempconv (devices_T1 + ws.Tnum) < 0)
  {
    werr (WERR_WARNING + WERR_AUTOCLOSE,
          "%s: could not start temperature conversion",
          devices_list[devices_T1 + ws.Tnum].menu_entry);
    return 0;
  }

  /* Copy scratchpad */
  Vad = Volt_Reading (FALSE, devices_T1 + ws.Tnum,
                      VOLT_READING_TRIES);
  if (Vad == -1.0F)
  {
    werr (WERR_WARNING + WERR_AUTOCLOSE,
          "Failed to read Vad of %s",
          devices_list[devices_T1 + ws.Tnum].menu_entry);
    return 0;
  }
  //msDelay(1) ;

  if (0 == Get_Temperature (devices_T1 + ws.Tnum, &T))
  {
    //if (T == 85.0F) return 0 ; /* Failed */
	assert((ws.Tnum>=0)&&(ws.Tnum<MAXTEMPS));
    ws.T[ws.Tnum] = T *
                     devices_list[devices_T1 + ws.Tnum].calib[0] +
                     devices_list[devices_T1 + ws.Tnum].calib[1];
  }
  else
  {
    werr (WERR_WARNING + WERR_AUTOCLOSE,
          "Failed to read temperature of %s",
          devices_list[devices_T1 + ws.Tnum].menu_entry);
    return 0;
  }

  return 1;
}


void
thermometer_new(devices_struct *dev)
{
  thermometer *T ;
  T = (thermometer *) calloc(1, sizeof(thermometer)) ;
  if (!T) exit(1) ;
  dev->local = T ;
  T->devtype = devtype_T ;
}

int thermometer_probe(void)
{
  thermometer *T ;
  
  T = (thermometer *) devices_list[devices_T1 + ws.Tnum].local ;
  
  if (!T)
    werr(1, "local data not present for %s", devices_list[devices_T1 + ws.Tnum].menu_entry) ;
  
  switch (T->state)
  {
    case thermometer_init: // Not started
    {
      int p = devices_portnum(devices_T1 + ws.Tnum) ;
    
      if (!devices_have_thermometer (ws.Tnum))
      {
        werr (0,
              "Attempt to start temp conv on missing thermometer (%d)",
              ws.Tnum);
        return -1 ; // Error
      }
      
      if (T->power_state == thermometer_power_unknown)
      {
        // Check thermometer power status
        if (devices_access(devices_T1+ws.Tnum))
        {
          /* send the Read Power Supply command */
          owTouchByte (p, 0xB4);
          T->power_state = (0 == owReadByte(p)) ?
              thermometer_power_bus : thermometer_power_vdd ;
          
          werr(WERR_DEBUG1, "%s power status %d", 
            devices_list[devices_T1+ws.Tnum].menu_entry, T->power_state);
        }
        else
        {
          werr(0, "Failed to access %s", devices_list[devices_T1+ws.Tnum].menu_entry);
          return -1 ;
        }
      }
      
      // What kind of thermometer is this?
      switch (devices_list[devices_T1+ws.Tnum].id[0])
      {
        // For slow thermometers, start tempconv, kick in strong pullup and wait
        case TEMP_FAMILY:
        case TEMP_FAMILY_DS1822:
        case TEMP_FAMILY_DS18B20:
        {
          /* access the device */
          if (!devices_access (devices_T1 + ws.Tnum))
            return -1;
        
          /* send the convert temperature command */
          owTouchByte (p, 0x44);
        
          if (T->power_state == thermometer_power_vdd)
          {
            /* Poll until temperature conversion finished */
//            int dtime = 0 ;
            while (owReadByte(p) == 0) 
            {
//              dtime += 10 ;
              msDelay(10) ;
            }
//            werr(WERR_DEBUG0, "dtime = %d", dtime) ;
            T->state = thermometer_done ;
            return 0 ; // Ok - ready to read
            //return weather_read_ws_end_tempconv() ;
          }
          else /* set the MicroLAN to strong pull-up */
          {
            if (owLevel (p, MODE_STRONG5) != MODE_STRONG5)
              return -1;
          
            /* Now sleep for 1 second */
          
            state_new (state_waiting_tempconv);
            T->state = thermometer_strong ;
            
            // Rest of this case handled in end_tempconv
          
            return 0 ;		/* OK - progstate will wait for 1s */
          }
        }
        
        // For fast sensors, start tempconv, poll busybyte, get result, and set for next sensor
        case SBATTERY_FAMILY:
        {
          if (!thermometer_read_ds2438_thermometer ())
            return -1 ; // Error
          
          T->state = thermometer_init ; // Ready for next time
          
          return 0 ;
        }
      }
  
      werr(1, "Unknown thermometer type") ;
      return -1 ;
    }
      
    case thermometer_conv: // Conversion running (with power)
    {
      /* access the device */
      if (!devices_access (devices_T1 + ws.Tnum))
        return -1;
        
      if (owReadByte(devices_portnum(devices_T1 + ws.Tnum)) == 0) return 0 ; // Not ready?
      
      T->state = thermometer_done ; // Ready to read
      break;
    }
        
      
    case thermometer_strong: // Conversion running (strong pullup)
    {
      // Check time since strong pullup was applied
      break;
    }
      
    case thermometer_done: // Conversion completed
      break ;
  }
  
  return -1 ;
}
