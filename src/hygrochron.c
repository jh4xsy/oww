/*
*  C Implementation: hygrochron
*
* Description: 
*
*
* Author: Simon Melhuish <simon@melhuish.info>, (C) 2005
*
* Copyright: See COPYING file that comes with this distribution
*
*/

/* Adapted from Dallas Semiconductor PD Library - humutil.c */

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
//
//  humutil.c - functions to set mission and mission states for DS1922.
//  Version 2.00
//

#include <time.h>
#include "math.h"
#include "hygrochron.h"
#include "devices.h"
//#include "mbee77.h"
//#include "pw77.h"

// Temperature resolution in degrees Celsius
double temperatureResolution = 0.5;

// max and min temperature
double maxTemperature = 85, minTemperature = -40;

// should we update the Real time clock?
SMALLINT updatertc = FALSE;


#define MAX_READ_RETRY_CNT  15

// *****************************************************************************
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Register addresses and control bits
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *****************************************************************************

/** Address of the Real-time Clock Time value*/
#define RTC_TIME  0x200
/** Address of the Real-time Clock Date value*/
#define RTC_DATE  0x203

/** Address of the Sample Rate Register */
#define SAMPLE_RATE  0x206 // 2 bytes, LSB first, MSB no greater than 0x3F

/** Address of the Temperature Low Alarm Register */
#define TEMPERATURE_LOW_ALARM_THRESHOLD  0x208
/** Address of the Temperature High Alarm Register */
#define TEMPERATURE_HIGH_ALARM_THRESHOLD  0x209

/** Address of the Data Low Alarm Register */
#define DATA_LOW_ALARM_THRESHOLD  0x20A
/** Address of the Data High Alarm Register */
#define DATA_HIGH_ALARM_THRESHOLD  0x20B

/** Address of the last temperature conversion's LSB */
#define LAST_TEMPERATURE_CONVERSION_LSB  0x20C
/** Address of the last temperature conversion's MSB */
#define LAST_TEMPERATURE_CONVERSION_MSB  0x20D

/** Address of the last data conversion's LSB */
#define LAST_DATA_CONVERSION_LSB  0x20E
/** Address of the last data conversion's MSB */
#define LAST_DATA_CONVERSION_MSB  0x20F

/** Address of Temperature Control Register */
#define TEMPERATURE_CONTROL_REGISTER  0x210
/** Temperature Control Register Bit: Enable Data Low Alarm */
#define TCR_BIT_ENABLE_TEMPERATURE_LOW_ALARM  0x01
/** Temperature Control Register Bit: Enable Data Low Alarm */
#define TCR_BIT_ENABLE_TEMPERATURE_HIGH_ALARM  0x02

/** Address of Data Control Register */
#define DATA_CONTROL_REGISTER  0x211
/** Data Control Register Bit: Enable Data Low Alarm */
#define DCR_BIT_ENABLE_DATA_LOW_ALARM  0x01
/** Data Control Register Bit: Enable Data High Alarm */
#define DCR_BIT_ENABLE_DATA_HIGH_ALARM  0x02

/** Address of Real-Time Clock Control Register */
#define RTC_CONTROL_REGISTER  0x212
/** Real-Time Clock Control Register Bit: Enable Oscillator */
#define RCR_BIT_ENABLE_OSCILLATOR  0x01
/** Real-Time Clock Control Register Bit: Enable High Speed Sample */
#define RCR_BIT_ENABLE_HIGH_SPEED_SAMPLE  0x02

/** Address of Mission Control Register */
#define MISSION_CONTROL_REGISTER  0x213
/** Mission Control Register Bit: Enable Temperature Logging */
#define MCR_BIT_ENABLE_TEMPERATURE_LOGGING  0x01
/** Mission Control Register Bit: Enable Data Logging */
#define MCR_BIT_ENABLE_DATA_LOGGING  0x02
/** Mission Control Register Bit: Set Temperature Resolution */
#define MCR_BIT_TEMPERATURE_RESOLUTION  0x04
/** Mission Control Register Bit: Set Data Resolution */
#define MCR_BIT_DATA_RESOLUTION  0x08
/** Mission Control Register Bit: Enable Rollover */
#define MCR_BIT_ENABLE_ROLLOVER  0x10
/** Mission Control Register Bit: Start Mission on Temperature Alarm */
#define MCR_BIT_START_MISSION_ON_TEMPERATURE_ALARM  0x20

/** Address of Alarm Status Register */
#define ALARM_STATUS_REGISTER  0x214
/** Alarm Status Register Bit: Temperature Low Alarm */
#define ASR_BIT_TEMPERATURE_LOW_ALARM  0x01
/** Alarm Status Register Bit: Temperature High Alarm */
#define ASR_BIT_TEMPERATURE_HIGH_ALARM  0x02
/** Alarm Status Register Bit: Data Low Alarm */
#define ASR_BIT_DATA_LOW_ALARM  0x04
/** Alarm Status Register Bit: Data High Alarm */
#define ASR_BIT_DATA_HIGH_ALARM  0x08
/** Alarm Status Register Bit: Battery On Reset */
#define ASR_BIT_BATTERY_ON_RESET  0x80

/** Address of General Status Register */
#define GENERAL_STATUS_REGISTER  0x215
/** General Status Register Bit: Sample In Progress */
#define GSR_BIT_SAMPLE_IN_PROGRESS  0x01
/** General Status Register Bit: Mission In Progress */
#define GSR_BIT_MISSION_IN_PROGRESS  0x02
/** General Status Register Bit: Conversion In Progress */
#define GSR_BIT_CONVERSION_IN_PROGRESS  0x04
/** General Status Register Bit: Memory Cleared */
#define GSR_BIT_MEMORY_CLEARED  0x08
/** General Status Register Bit: Waiting for Temperature Alarm */
#define GSR_BIT_WAITING_FOR_TEMPERATURE_ALARM  0x10
/** General Status Register Bit: Forced Conversion In Progress */
#define GSR_BIT_FORCED_CONVERSION_IN_PROGRESS  0x20

/** Address of the Mission Start Delay */
#define MISSION_START_DELAY  0x216 // 3 bytes, LSB first

/** Address of the Mission Timestamp Time value*/
#define MISSION_TIMESTAMP_TIME  0x219
/** Address of the Mission Timestamp Date value*/
#define MISSION_TIMESTAMP_DATE  0x21C

/** Address of Device Configuration Register */
#define DEVICE_CONFIGURATION_BYTE  0x226

// 1 byte, alternating ones and zeroes indicates passwords are enabled
/** Address of the Password Control Register. */
#define PASSWORD_CONTROL_REGISTER  0x227

// 8 bytes, write only, for setting the Read Access Password
/** Address of Read Access Password. */
#define READ_ACCESS_PASSWORD  0x228

// 8 bytes, write only, for setting the Read Access Password
/** Address of the Read Write Access Password. */
#define READ_WRITE_ACCESS_PASSWORD  0x230

// 3 bytes, LSB first
/** Address of the Mission Sample Count */
#define MISSION_SAMPLE_COUNT  0x220

// 3 bytes, LSB first
/** Address of the Device Sample Count */
#define DEVICE_SAMPLE_COUNT  0x223

// first year that calendar starts counting years from
#define FIRST_YEAR_EVER  2000

// maximum size of the mission log
#define MISSION_LOG_SIZE  8192

// mission log size for odd combination of resolutions (i.e. 8-bit temperature
// & 16-bit data or 16-bit temperature & 8-bit data
#define ODD_MISSION_LOG_SIZE  7680

#define TEMPERATURE_CHANNEL  0
#define DATA_CHANNEL         1

/** 1-Wire command for Clear Memory With Password */
#define CLEAR_MEMORY_PW_COMMAND     0x96
/** 1-Wire command for Start Mission With Password */
#define START_MISSION_PW_COMMAND    0xCC
/** 1-Wire command for Stop Mission With Password */
#define STOP_MISSION_PW_COMMAND     0x33
/** 1-Wire command for Forced Conversion */
#define FORCED_CONVERSION           0x55

// Functions defined locally

SMALLINT readPageCRCEE77(SMALLINT bank, int portnum, int devnum, int page, uchar *buff);

int
hygrochron_read(int devnum, float *RH, float *Trh)
{
  configLog config;
  uchar state[96];
  double val,valsq,error;
  int portnum ;
  
  config.configByte = 0x20 ;
  portnum = devices_portnum(devnum) ;
  
  if(readDevice(portnum,devnum,&state[0],&config)==0)
  {
    if(0!=doTemperatureConvert(portnum,devnum,&state[0]))
    {
      werr(WERR_WARNING, "Hygrochron temperature conversion failed") ;
      return -1 ;
    }
    else
    {
      config.useTemperatureCalibration = 1;

      val = decodeTemperature(&state[12],2,FALSE,config);
      valsq = val*val;
      error = config.tempCoeffA*valsq +
          config.tempCoeffB*val + config.tempCoeffC;
      val = val - error;
      *Trh = val ;
      
      config.useHumidityCalibration = 1;

      val = decodeHumidity(&state[14],2,FALSE,config);
      valsq = val*val;
      error = config.humCoeffA*valsq +
          config.humCoeffB*val + config.humCoeffC;
      val = val - error;

      if(val < 0.0)
          val = 0.0;

      *RH = val ;
    }
  }
  else
  {
      return -1 ;
  }

  werr(WERR_DEBUG1, "Hygrochron %f %% %f C", *RH, *Trh) ;
  
  return 0 ;
}

// *****************************************************************************
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Sensor read/write
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *****************************************************************************

/**
 * Retrieves the 1-Wire device sensor state.  This state is
 * returned as a byte array.  Pass this byte array to the 'get'
 * and 'set' methods.  If the device state needs to be changed then call
 * the 'writeDevice' to finalize the changes.
 *
 * @return 1-Wire device sensor state
 */
SMALLINT readDevice(int portnum, int devnum, uchar *buffer, configLog *config)
{
   int i;
   int retryCnt = MAX_READ_RETRY_CNT;
   uchar temp_buff[96];
   // reference humidities that the calibration was calculated over
   double ref1 = 20.0, ref2 = 60.0, ref3 = 90.0;
   // the average value for each reference point
   double read1 = 0.0, read2 = 0.0, read3 = 0.0;
   // the average error for each reference point
   double error1 = 0.0, error2 = 0.0, error3 = 0.0;
   double ref1sq, ref2sq, ref3sq;

   config->adDeviceBits = 10;
   config->adReferenceVoltage = 5.02;

   do
   {
      if(readPageCRCEE77(0,portnum,devnum,16,&temp_buff[0]) &&
         readPageCRCEE77(0,portnum,devnum,17,&temp_buff[32]) &&
         readPageCRCEE77(0,portnum,devnum,18,&temp_buff[64]))
      {
         for(i=0;i<96;i++)
            buffer[i] = temp_buff[i];
      }
      else
      {
         retryCnt++;
      }
   }
   while(retryCnt<MAX_READ_RETRY_CNT);

   switch(config->configByte)
   {
      case 0x00:
         config->lowTemp  = -40;
         config->highTemp = 125;
         break;

      case 0x20:
         config->lowTemp  = -40;
         config->highTemp = 125;
         config->hasHumidity = TRUE;
         break;

      case 0x40:
         config->lowTemp  = -40;
         config->highTemp = 125;
         break;

      case 0x60:
         config->lowTemp  = 0;
         config->highTemp = 125;
         break;

      default:
         config->lowTemp  = -40;
         config->highTemp = 125;
         break;
   }

   if(config->hasHumidity)
   {
      config->useHumidityCalibration = TRUE;

      ref1 = decodeHumidity(&buffer[72],2,TRUE,*config);
      read1 = decodeHumidity(&buffer[74],2,TRUE,*config);
      error1 = read1 - ref1;
      ref2 = decodeHumidity(&buffer[76],2,TRUE,*config);
      read2 = decodeHumidity(&buffer[78],2,TRUE,*config);
      error2 = read2 - ref2;
      ref3 = decodeHumidity(&buffer[80],2,TRUE,*config);
      read3 = decodeHumidity(&buffer[82],2,TRUE,*config);
      error3 = read3 - ref3;

      ref1sq = ref1*ref1;
      ref2sq = ref2*ref2;
      ref3sq = ref3*ref3;

      config->humCoeffB =
         ( (ref2sq-ref1sq)*(error3-error1) + ref3sq*(error1-error2)
           + ref1sq*(error2-error1) ) /
         ( (ref2sq-ref1sq)*(ref3-ref1) + (ref3sq-ref1sq)*(ref1-ref2) );
      config->humCoeffA =
         ( error2 - error1 + config->humCoeffB*(ref1-ref2) ) /
         ( ref2sq - ref1sq );
      config->humCoeffC =
         error1 - config->humCoeffA*ref1sq - config->humCoeffB*ref1;
   }

   config->useTemperatureCalibration = TRUE;

   ref2 = decodeHumidity(&buffer[64],2,TRUE,*config);
   read2 = decodeHumidity(&buffer[66],2,TRUE,*config);
   error2 = read2 - ref2;
   ref3 = decodeHumidity(&buffer[68],2,TRUE,*config);
   read3 = decodeHumidity(&buffer[70],2,TRUE,*config);
   error3 = read3 - ref3;
   ref1 = 60.0;
   error1 = error2;
   read1 = ref1 + error1;

   ref1sq = ref1*ref1;
   ref2sq = ref2*ref2;
   ref3sq = ref3*ref3;

   config->tempCoeffB =
      ( (ref2sq-ref1sq)*(error3-error1) + ref3sq*(error1-error2)
        + ref1sq*(error2-error1) ) /
      ( (ref2sq-ref1sq)*(ref3-ref1) + (ref3sq-ref1sq)*(ref1-ref2) );
   config->tempCoeffA =
      ( error2 - error1 + config->tempCoeffB*(ref1-ref2) ) /
      ( ref2sq - ref1sq );
   config->tempCoeffC =
         error1 - config->tempCoeffA*ref1sq - config->tempCoeffB*ref1;

   config->useTempCalforHumidity = FALSE;

   return 0 ; /* Ok */
}

/**
 * Reads a single byte from the DS1922.  Note that the preferred manner
 * of reading from the DS1922 Thermocron is through the <code>readDevice()</code>
 * method or through the <code>MemoryBank</code> objects returned in the
 * <code>getMemoryBanks()</code> method.
 *
 * memAddr the address to read from  (in the range of 0x200-0x21F)
 */
uchar readByte(int portnum, int devnum, int memAddr)
{
   uchar buffer[32];
   int page;

   page = memAddr/32;

   readPageCRCEE77(0,portnum,devnum,page,&buffer[0]);

   return buffer[memAddr%32];
}


/**
 * Gets the status of the specified flag from the specified register.
 * This method actually communicates with the Thermocron.  To improve
 * performance if you intend to make multiple calls to this method,
 * first call readDevice() and use the
 * getFlag(int, byte, byte[]) method instead.</p>
 *
 * The DS1922 Thermocron has two sets of flags.  One set belongs
 * to the control register.  When reading from the control register,
 * valid values for bitMask.
 *
 * @param register address of register containing the flag (valid values
 * are CONTROL_REGISTER and STATUS_REGISTER)
 * @param bitMask the flag to read (see above for available options)
 *
 * @return the status of the flag, where true
 * signifies a "1" and false signifies a "0"
 */
SMALLINT getFlag (int portnum, int devnum, int reg, uchar bitMask)
{
   return ((readByte(portnum,devnum,reg) & bitMask) != 0);
}

// *****************************************************************************
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Temperature Interface Functions
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *****************************************************************************

/**
 * Performs a temperature conversion.  Use the <code>state</code>
 * information to calculate the conversion time.
 *
 * @param  state byte array with device state information
 *
 */
SMALLINT doTemperatureConvert(int portnum, int devnum, uchar *state)
{
   uchar buffer[2];

   /* check for mission in progress */
   if(getFlag(portnum,devnum,GENERAL_STATUS_REGISTER,GSR_BIT_MISSION_IN_PROGRESS))
   {
      werr(0, "Can't force read during a mission.");
      return -1;
   }

   /* check that the RTC is running */
   if(!getFlag(portnum,devnum,RTC_CONTROL_REGISTER,RCR_BIT_ENABLE_OSCILLATOR))
   {
      werr(0, "Can't force read if the oscillator is not enabled.");
      return -1;
   }

   if(!devices_access(devnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return -1;
   }

   // perform the conversion
   buffer[0] = (uchar) FORCED_CONVERSION;
   buffer[1] = 0xFF;

   // send block (check copy indication complete)
   if(!owBlock(portnum,FALSE,&buffer[0],2))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return -1;
   }

   msDelay(750);

   // grab the data
   state[LAST_TEMPERATURE_CONVERSION_LSB&0x3F]
      = readByte(portnum,devnum,LAST_TEMPERATURE_CONVERSION_LSB);
   state[LAST_TEMPERATURE_CONVERSION_MSB&0x3F]
      = readByte(portnum,devnum,LAST_TEMPERATURE_CONVERSION_MSB);
   // grab RH the data
   state[LAST_DATA_CONVERSION_LSB&0x3F]
      = readByte(portnum,devnum,LAST_DATA_CONVERSION_LSB);
   state [LAST_DATA_CONVERSION_MSB&0x3F]
      = readByte(portnum,devnum,LAST_DATA_CONVERSION_MSB);

   return 0;
}

double getADVoltage(uchar *data, int length, SMALLINT reverse, configLog config)
{
   double dval;
   // get the 10-bit value of vout
   int ival = 0;

   if(reverse && length==2)
   {
      ival = ((data[0]&0x0FF)<<(config.adDeviceBits-8));
      ival |= ((data[1]&0x0FF)>>(16-config.adDeviceBits));
   }
   else if(length==2)
   {
      ival = ((data[1]&0x0FF)<<(config.adDeviceBits-8));
      ival |= ((data[0]&0x0FF)>>(16-config.adDeviceBits));
   }
   else
   {
      ival = ((data[0]&0x0FF)<<(config.adDeviceBits-8));
   }

   dval = (ival*config.adReferenceVoltage)/(1<<config.adDeviceBits);

   return dval;
}

//--------
//-------- Clock 'set' Methods
//--------

/**
 * Set the time in the DS1922 time register format.
 */
void setTime(int timeReg, int hours, int minutes, int seconds,
             SMALLINT AMPM, uchar *state)
{
   uchar upper, lower;

   /* format in bytes and write seconds */
   upper            = (uchar) (((seconds / 10) << 4) & 0xf0);
   lower            = (uchar) ((seconds % 10) & 0x0f);
   state[timeReg++] = (uchar) (upper | lower);

   /* format in bytes and write minutes */
   upper            = (uchar) (((minutes / 10) << 4) & 0xf0);
   lower            = (uchar) ((minutes % 10) & 0x0f);
   state[timeReg++] = (uchar) (upper | lower);

   /* format in bytes and write hours/(12/24) bit */
   if (AMPM)
   {
      upper = 0x04;

      if (hours > 11)
         upper = (uchar) (upper | 0x02);

      // this next logic simply checks for a decade hour
      if (((hours % 12) == 0) || ((hours % 12) > 9))
         upper = (uchar) (upper | 0x01);

      if (hours > 12)
         hours = hours - 12;

      if (hours == 0)
         lower = 0x02;
      else
         lower = (uchar) ((hours % 10) & 0x0f);
   }
   else
   {
      upper = (uchar) (hours / 10);
      lower = (uchar) (hours % 10);
   }

   upper          = (uchar) ((upper << 4) & 0xf0);
   lower          = (uchar) (lower & 0x0f);
   state[timeReg] = (uchar) (upper | lower);
}

/**
 * Set the current date in the DS1922's real time clock.
 *
 * year - The year to set to, i.e. 2001.
 * month - The month to set to, i.e. 1 for January, 12 for December.
 * day - The day of month to set to, i.e. 1 to 31 in January, 1 to 30 in April.
 */
void setDate (int timeReg, int year, int month, int day, uchar *state)
{
   uchar upper, lower;

   /* write the day byte (the upper holds 10s of days, lower holds single days) */
   upper             = (uchar) (((day / 10) << 4) & 0xf0);
   lower             = (uchar) ((day % 10) & 0x0f);
   state[timeReg++] = (uchar) (upper | lower);

   /* write the month bit in the same manner, with the MSBit indicating
      the century (1 for 2000, 0 for 1900) */
   upper             = (uchar) (((month / 10) << 4) & 0xf0);
   lower             = (uchar) ((month % 10) & 0x0f);
   state[timeReg++] = (uchar) (upper | lower);

   // now write the year
   year             = year + 1900;
   year             = year - FIRST_YEAR_EVER;

   if(year>100)
   {
      state[timeReg-1] |= 0x80;
      year -= 100;
   }
   upper            = (uchar) (((year / 10) << 4) & 0xf0);
   lower            = (uchar) ((year % 10) & 0x0f);
   state[timeReg]  = (uchar) (upper | lower);
}




/**
 * helper method for decoding temperature values
 */
double decodeTemperature(uchar *data, int length, SMALLINT reverse, configLog config)
{
   double whole, fraction = 0;

   if(reverse && length==2)
   {
      fraction = ((data[1]&0x0FF)/512.0);
      whole = (data[0]&0x0FF)/2.0 + (config.lowTemp-1);
   }
   else if(length==2)
   {
      fraction = ((data[0]&0x0FF)/512.0);
      whole = (data[1]&0x0FF)/2.0 + (config.lowTemp-1);
   }
   else
   {
      whole = (data[0]&0x0FF)/2.0 + (config.lowTemp-1);
   }

   return whole + fraction;
}


/**
 * helper method for decoding humidity values
 */
double decodeHumidity(uchar *data, int length, SMALLINT reverse, configLog config)
{
   double val;

   // get the 10-bit value of Vout
   val = getADVoltage(data, length, reverse, config);

   // convert Vout to a humidity reading
   // this formula is from HIH-3610 sensor datasheet
   val = (val-.958)/.0307;

   return val;
}

// General command defines
#define READ_MEMORY_PSW_COMMAND  0x69
#define WRITE_SCRATCHPAD_COMMAND 0x0F
#define READ_SCRATCHPAD_COMMAND  0xAA
#define COPY_SCRATCHPAD_COMMAND  0x55
// Local defines
#define SIZE        32768
#define PAGE_LENGTH 64
#define PAGE_LENGTH_HYGRO 32

/**
 * Read a complete memory page with CRC verification provided by the
 * device.  Not supported by all devices.  See the method
 * 'hasPageAutoCRCEE()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * devnum   the device number for the part.
 * psw      8 byte password
 * page     the page to read
 * rd_cont  if 'true' then device read is continued without
 *          re-selecting.  This can only be used if the new
 *          read() continious where the last one led off
 *          and it is inside a 'beginExclusive/endExclusive'
 *          block.
 * buff     byte array containing data that was read
 *
 * @return - returns '0' if the read page wasn't completed.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageCRCEE77(SMALLINT bank, int portnum, int devnum,
                         int page, uchar *buff)
{
   SMALLINT i, send_len=0, lsCRC16;
   uchar  raw_buf[15];
   int str_add;
   ushort lastcrc16;

  //uchar psw[8] ;

  /* Null password */
  //for(i=length;i<8;i++) psw[i] = 0x00;

   // check if not continuing on the next page
/*   if (!rd_cont)
   {*/
      // set serial number of device to read
      //owSerialNum(portnum,SNum,FALSE);

      // select the device
      if(!devices_access(devnum))
      //if (!owAccess(portnum))
      {
         OWERROR(OWERROR_DEVICE_SELECT_FAIL);
         return FALSE;
      }

      // command, address, offset, password (except last byte)
      raw_buf[send_len++] = READ_MEMORY_PSW_COMMAND;
      str_add = page * PAGE_LENGTH_HYGRO;
      
      raw_buf[send_len++] = str_add & 0xFF;
      raw_buf[send_len++] = ((str_add & 0xFFFF) >> 8) & 0xFF;

      // calculate the CRC16
      setcrc16(portnum,0);
      for(i = 0; i < send_len; i++)
         lastcrc16 = docrc16(portnum,raw_buf[i]);

      for (i = 0; i < 8; i++)
         raw_buf[send_len++] = '\0' ; //psw[i];


      if(!owBlock(portnum,FALSE,raw_buf,send_len))
      {
        OWERROR(OWERROR_BLOCK_FAILED);
        return FALSE;
      }

   // set the read bytes
   for(i=0; i<PAGE_LENGTH_HYGRO; i++)
     buff[i] = 0xFF;

   // read the data
   if(!owBlock(portnum,FALSE,buff,PAGE_LENGTH_HYGRO))
   {
     OWERROR(OWERROR_BLOCK_FAILED);
     return FALSE;
   }
    // read the first CRC16 byte
   lsCRC16 = owReadByte(portnum);

   // calculate CRC on data read
   for(i = 0; i < PAGE_LENGTH_HYGRO; i++)
      lastcrc16 = docrc16(portnum,buff[i]);

   // check lsByte of the CRC
   if ((lastcrc16 & 0xFF) != (~lsCRC16 & 0xFF))
   {
      OWERROR(OWERROR_CRC_FAILED);
      return FALSE;
   }

   return TRUE;
}
