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
//  swt12.c - Modifies Channel A and B and returns info byte data for
//            the DS2406 and DS2407.
//  version 2.00


// Include files
#include <stdio.h>
#include "ownet.h"
#include "swt12.h"
#include "devices.h"
#include "werr.h"

//----------------------------------------------------------------------
//  SUBROUTINE - ReadSwitch12
//
//      This routine gets the Channel Info Byte and returns it.
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number was provided to
//                   OpenCOM to indicate the port number.
//      'ClearActivity' - To reset the button
//
//      Returns: (-1) If the Channel Info Byte could not be read.
//                       (Info Byte) If the Channel Info Byte could be read.
//
int
ReadSwitch12 (int device, int ClearActivity)
{
	int rt = -1;		//this is the return value depending if the byte was read
	int trans_cnt = 0;	//this is the counter for the number of bytes to send
	uchar transfer[30];	//this is the whole block of byte info
	ushort lastcrc16;

	// access and verify it is there
	//MLanSerialNum (devices_list[device].id, FALSE);
	//if (MLanAccess ())
  if (devices_access(device))
	{
		// reset CRC
		//CRC16 = 0;
		setcrc16(devices_portnum(device),0);

		// channel access command
		transfer[trans_cnt++] = 0xF5;
		//crc16 (0xF5);
		lastcrc16 = docrc16(devices_portnum(device),0xF5);

		// control bytes
		if (ClearActivity)
		{
			transfer[trans_cnt++] = 0xD5;
			lastcrc16 = docrc16(devices_portnum(device),0xD5);
		}
		else
		{
			transfer[trans_cnt++] = 0x55;
			lastcrc16 = docrc16(devices_portnum(device),0x55);
		}

		transfer[trans_cnt++] = 0xFF;
		lastcrc16 = docrc16(devices_portnum(device),0xFF);

		// read the info byte
		transfer[trans_cnt++] = 0xFF;

		// dummy data
		transfer[trans_cnt++] = 0xFF;
		transfer[trans_cnt++] = 0xFF;
		transfer[trans_cnt++] = 0xFF;

		if (owBlock (devices_portnum(device),FALSE, transfer, trans_cnt))
		{
			rt = transfer[3];
			// read a dummy read byte and CRC16
			lastcrc16 = docrc16 (devices_portnum(device),transfer[trans_cnt - 4]);
			lastcrc16 = docrc16 (devices_portnum(device),transfer[trans_cnt - 3]);
			lastcrc16 = docrc16 (devices_portnum(device),transfer[trans_cnt - 2]);
			lastcrc16 = docrc16 (devices_portnum(device),transfer[trans_cnt - 1]);
			if(lastcrc16 != 0xB001)
				rt = -1;
		}
	}
	else
		rt = -1;

	return rt;
}


//----------------------------------------------------------------------
//      SUBROUTINE - SetSwitch12
//
//  This routine sets the channel state of the specified DS2406
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number was provided to
//                 OpenCOM to indicate the port number.
// 'SerialNum'   - Serial Number of DS2406 to set the switch state
// 'State'       - Is a type containing what to set A and/or B to.  It
//                                 also contains the other fields that maybe written later
//
//  Returns: TRUE(1)  State of DS2406 set and verified
//           FALSE(0) could not set the DS2406, perhaps device is not
//                    in contact
//
int
SetSwitch12 (int device, SwitchProps State)
{
	ushort st ;
	int rt = FALSE;
	uchar send_block[30];
	int send_cnt = 0;
   ushort lastcrc16;

   setcrc16(devices_portnum(device),0);

	// set the device serial number to the counter device
	//owSerialNum(devices_portnum(device),SerialNum,FALSE);
	//MLanSerialNum (devices_list[device].id, FALSE);

	// access the device
	if (devices_access(device))
	{
    int status ;
    
    //status = ReadStatusSwitch12(device) ;
    //werr(WERR_DEBUG0, "Status 0x%x", status) ;
    //st = status & 0x9f ;
    
		// create a block to send that reads the counter
		// write status command
		send_block[send_cnt++] = 0x55;
		lastcrc16 = docrc16 (devices_portnum(device),0x55);

		// address of switch state
		send_block[send_cnt++] = 0x07;
		lastcrc16 = docrc16 (devices_portnum(device),0x07);
		send_block[send_cnt++] = 0x00;
		lastcrc16 = docrc16 (devices_portnum(device),0x00);

		// write state
		st = 0x1F;
		if (!State.Chan_B)
			st |= 0x40;
		if (!State.Chan_A)
			st |= 0x20;
		//if (State.Supply)
		//	st |= 0x80;
		// more ifs can be added here for the other fields.

		send_block[send_cnt++] = (uchar) st;
		lastcrc16 = docrc16 (devices_portnum(device),st);

		// read CRC16
		send_block[send_cnt++] = 0xFF;
		send_block[send_cnt++] = 0xFF;

		// now send the block
		if (owBlock (devices_portnum(device),FALSE, send_block, send_cnt))
		{
			// perform the CRC16 on the last 2 bytes of packet
			lastcrc16 = docrc16 (devices_portnum(device),send_block[send_cnt - 2]);
			lastcrc16 = docrc16 (devices_portnum(device),send_block[send_cnt - 1]);

			// verify crc16 is correct
			if (lastcrc16 == 0xB001)
				rt = TRUE;
			else
				werr (WERR_WARNING+WERR_AUTOCLOSE, "CRC16 failed: %s",
				      devices_list[device].menu_entry);
      
      owReadByte(devices_portnum(device)) ;
      status = owReadByte(devices_portnum(device)) ;
//      werr(WERR_DEBUG0, "Got back 0x%x", status) ;
      
      //status = ReadStatusSwitch12(device) ;
      //werr(WERR_DEBUG0, "ReadStatusSwitch12 got 0x%x", status) ;
      if ((status & 0x60) == (st & 0x60))
      {
//        werr(WERR_DEBUG0, "Status set correctly") ;
        rt = TRUE ;
      }
      else
      {
        werr(WERR_DEBUG0, "Status set incorrectly") ;
        rt = FALSE;
      }
        

			//MLanWriteByte(0xff) ; /* Copy the status byte */
			//printf("SetSwitch12 %s info byte 0x%x\n",
			//  devices_list[device].menu_entry, MLanReadByte()) ;
		}
		else
			werr (WERR_WARNING+WERR_AUTOCLOSE, "MLanBlock failed: %s",
			      devices_list[device].menu_entry);
	}
	else
	{
		werr (WERR_WARNING+WERR_AUTOCLOSE, "devices_access failed: %s",
		      devices_list[device].menu_entry);
	}

	// return the result flag rt
	return rt;
}

int
WriteSwitch12 (int device, ushort st)
{
	int rt =0;
	uchar send_block[30];
	int send_cnt = 0;
   ushort lastcrc16;

   setcrc16(devices_portnum(device),0);

	// set the device serial number to the counter device
	//owSerialNum(devices_portnum(device),SerialNum,FALSE);
	//MLanSerialNum (devices_list[device].id, FALSE);

	// access the device
	if (devices_access(device))
	//if (MLanAccess ())
	{
    int status ;
    
    //status = ReadStatusSwitch12(device) ;
    //werr(WERR_DEBUG0, "Status 0x%x", status) ;
    //st = status & 0x9f ;
    
		// create a block to send that reads the counter
		// write status command
		send_block[send_cnt++] = 0x55;
		lastcrc16 = docrc16 (devices_portnum(device),0x55);

		// address of switch state
		send_block[send_cnt++] = 0x07;
		lastcrc16 = docrc16 (devices_portnum(device),0x07);
		send_block[send_cnt++] = 0x00;
		lastcrc16 = docrc16 (devices_portnum(device),0x00);

		send_block[send_cnt++] = (uchar) st;
		lastcrc16 = docrc16 (devices_portnum(device),st);

		// read CRC16
		send_block[send_cnt++] = 0xFF;
		send_block[send_cnt++] = 0xFF;

		// now send the block
		if (owBlock (devices_portnum(device), FALSE, send_block, send_cnt))
		{
			// perform the CRC16 on the last 2 bytes of packet
			lastcrc16 = docrc16 (devices_portnum(device),send_block[send_cnt - 2]);

			// verify crc16 is correct
			lastcrc16 = docrc16 (devices_portnum(device),send_block[send_cnt - 1]) ;
			if (lastcrc16 == 0xB001)
				rt = 0;
			else
				werr (WERR_WARNING+WERR_AUTOCLOSE, "CRC16 failed: %s",
				      devices_list[device].menu_entry);
      
      owReadByte(devices_portnum(device)) ;
      status = owReadByte(devices_portnum(device)) ;
//      werr(WERR_DEBUG0, "Got back 0x%x", status) ;
      
      //status = ReadStatusSwitch12(device) ;
      //werr(WERR_DEBUG0, "ReadStatusSwitch12 got 0x%x", status) ;
      if ((status & 0x60) == (st & 0x60))
      {
//        werr(WERR_DEBUG0, "Status set correctly") ;
        rt = status ;
      }
      else
      {
        werr(WERR_DEBUG0, "Status set incorrectly") ;
        rt = -1;
      }
        

			//MLanWriteByte(0xff) ; /* Copy the status byte */
			//printf("SetSwitch12 %s info byte 0x%x\n",
			//  devices_list[device].menu_entry, MLanReadByte()) ;
		}
		else
			werr (WERR_WARNING+WERR_AUTOCLOSE, "MLanBlock failed: %s",
			      devices_list[device].menu_entry);
	}
	else
	{
		werr (WERR_WARNING+WERR_AUTOCLOSE, "devices_access failed: %s",
		      devices_list[device].menu_entry);
	}

	// return the result flag rt
	return rt;
}

uchar
SwitchSetLatchState12 (int chan, int latchState, int doSmart, uchar state)
{
	//char buffer[512] = "" ;
	uchar new_state = 0x1f;
  new_state = state ;
	/*if (state & 0x01)
		new_state |= 0x20;
	if (state & 0x02)
		new_state |= 0x40;*/

	if (chan == 0)
	{
		/* Channel A */
		if (latchState == 0)
			new_state |= 0x20;
		else
			new_state &= 0xdf;
	}
	else
	{
		/* Channel B */
		if (latchState == 0)
			new_state |= 0x40;
		else
			new_state &= 0xbf;
	}
	//SwitchStateToString12(*state, buffer);
	//printf("SwitchSetLatchState12 state = 0x%x\n%s", *state, buffer);
	return new_state;
}

//----------------------------------------------------------------------
//      SUBROUTINE - SwitchStateToString12
//
//  This routine uses the info byte to return a string with all the data.
//
// 'infobyte'   - This is the information byte data from the hardware.
// 'outstr'     - This will be the output string.  It gets set in the
//                                    the procedure.
//
// return the length of the string
//
int
SwitchStateToString12 (int infobyte, char *outstr)
{

	int cnt = 0;

	if (infobyte & 0x40)
	{
		cnt += sprintf (outstr + cnt, "%s", "Channel A and B\n");

		if (infobyte & 0x80)
			cnt += sprintf (outstr + cnt, "%s", "Supply\n");
		else
			cnt += sprintf (outstr + cnt, "%s", "No Supply\n");

		if (infobyte & 0x20)
			cnt += sprintf (outstr + cnt, "%s",
					"Activity on PIO-B\n");
		else
			cnt += sprintf (outstr + cnt, "%s",
					"No activity on PIO-B\n");

		if (infobyte & 0x10)
			cnt += sprintf (outstr + cnt, "%s",
					"Activity on PIO-A\n");
		else
			cnt += sprintf (outstr + cnt, "%s",
					"No activity on PIO-A\n");

		if (infobyte & 0x08)
			cnt += sprintf (outstr + cnt, "%s",
					"Hi level on PIO B\n");
		else
			cnt += sprintf (outstr + cnt, "%s",
					"Lo level on PIO B\n");

		if (infobyte & 0x04)
			cnt += sprintf (outstr + cnt, "%s",
					"Hi level on PIO A\n");
		else
			cnt += sprintf (outstr + cnt, "%s",
					"Lo level on PIO A\n");

		if (infobyte & 0x02)
			cnt += sprintf (outstr + cnt, "%s",
					"Channel B off\n");
		else
			cnt += sprintf (outstr + cnt, "%s", "Channel B on\n");

		if (infobyte & 0x01)
			cnt += sprintf (outstr + cnt, "%s",
					"Channel A off\n");
		else
			cnt += sprintf (outstr + cnt, "%s", "Channel A on\n");
	}
	else
	{
		cnt += sprintf (outstr + cnt, "%s", "Channel A\n");

		if (infobyte & 0x80)
			cnt += sprintf (outstr + cnt, "%s", "Supply\n");
		else
			cnt += sprintf (outstr + cnt, "%s", "No Supply\n");

		if (infobyte & 0x10)
			cnt += sprintf (outstr + cnt, "%s",
					"Activity on PIO-A\n");
		else
			cnt += sprintf (outstr + cnt, "%s",
					"No activity on PIO-A\n");

		if (infobyte & 0x04)
			cnt += sprintf (outstr + cnt, "%s",
					"Hi level on PIO A\n");
		else
			cnt += sprintf (outstr + cnt, "%s",
					"Lo level on PIO A\n");

		if (infobyte & 0x01)
			cnt += sprintf (outstr + cnt, "%s",
					"Channel A off\n");
		else
			cnt += sprintf (outstr + cnt, "%s", "Channel A on\n");

	}

	return cnt;
}
