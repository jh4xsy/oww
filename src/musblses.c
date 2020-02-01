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
//  usbwses.c - Acquire and release a Session on the 1-Wire Net using the
//              USB interface DS2490.
//
//  Version: 3.00
//
//  History:
//

// Version for libowpd by Dr. Simon J. Melhuish
// simon@melhuish.info

#include <string.h>
#include "ownet.h"
#include "libusbds2490.h"

// handles to USB ports
struct usb_dev_handle* usbhnd[MAX_PORTNUM];
struct usb_device *usb_dev_list[MAX_PORTNUM];
//static SMALLINT usbhnd_init = 0;
int usb_num_devices = -1;
//int fd[MAX_PORTNUM];
//SMALLINT fd_init = 0;

//---------------------------------------------------------------------------
// Initialize the USB system
//
static void USBInit(void)
{
	struct usb_bus *bus;
	struct usb_device *dev;
    
  static int USBInitDone = 0;
  int i ;
  
  if (USBInitDone != 0) return ;

	// initialize USB subsystem
	usb_init();
	usb_set_debug(0);
	usb_find_busses();
	usb_find_devices();
	
	for (i=0; i<MAX_PORTNUM; ++i)
	  usbhnd[i] = NULL ; 

//      	printf("bus/device  idVendor/idProduct\n");
	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
//			printf("%s/%s     %04X/%04X\n", bus->dirname, dev->filename,
//					dev->descriptor.idVendor, dev->descriptor.idProduct);
			if (dev->descriptor.idVendor == 0x04FA &&
					dev->descriptor.idProduct == 0x2490) {
//				printf("Found DS2490 device #%d at %s/%s\n", usb_num_devices + 1,
//						bus->dirname, dev->filename);
				//++usb_num_devices ;
				usb_dev_list[++usb_num_devices] = dev;
			}
		}
	}

	USBInitDone = 1 ;
}

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net using a USB port and a DS2490 based
// adapter.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'port_zstr'  - zero terminated port name.  For this platform
//                use format 'USB' or 'USB:X' where X is the port number.
//
// Returns: TRUE - success, COM port opened
//
SMALLINT owAcquire_DS9490(int portnum, char *port_zstr)
{
	/* check the port string */
	if (strcmp (port_zstr, "USB")) 
	{
	  /* Not simple "USB" port name */
	  int num ;
	  /* Check for "USB:p" style port name */
	  if (sscanf(port_zstr, "USB:%d", &num) == 1)
	  {
	    /* Check that port number requests match */
	    if (num != portnum)
	    {
//               printf("owAcquire_DS9490 port number request didn't match: %d, %d\n", num, portnum);
              OWERROR(OWERROR_PORTNUM_ERROR);
	      return 0 ;
	    }
	  }
	  else
	  {
//             printf("owAcquire_DS9490 didn't find num\n");
            OWERROR(OWERROR_PORTNUM_ERROR);
     	    return 0;
	  }
	}
	
  USBInit() ;
  
  if ((portnum>=MAX_PORTNUM)||(portnum<0))
  //printf("Assert about to fail in owAcquire_DS9490\n");

  OWASSERT( portnum<MAX_PORTNUM && portnum>=0 && !usbhnd[portnum],
             OWERROR_PORTNUM_ERROR, FALSE );

	/* check to see if opening the device is valid */
	if (usbhnd[portnum] != NULL) {
          OWERROR(OWERROR_LIBUSB_DEVICE_ALREADY_OPENED);
		//strcpy(return_msg, "Device allready open\n");
		return FALSE;
	}

	/* open the device */
	usbhnd[portnum] = usb_open(usb_dev_list[portnum]);
	if (usbhnd[portnum] == NULL) {
//           printf("owAcquire_DS9490 : open the device - usbhnd[%d]\n",portnum);
          OWERROR(OWERROR_LIBUSB_OPEN_FAILED);
		//strcpy(return_msg, "Failed to open usb device\n");
// 		printf("%s\n", usb_strerror());
		return FALSE;
	}

#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
	usb_detach_kernel_driver_np(usbhnd[portnum],0);
#endif /* LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP */
	
	/* set the configuration */
	if (usb_set_configuration(usbhnd[portnum], 1) < 0) {
// 		printf("Failed to set configuration\n");
// 		printf("%s\n", usb_strerror());
		usb_close(usbhnd[portnum]);
                usbhnd[portnum] = NULL ;
//                 printf("owAcquire_DS9490 usb_set_configuration failed on port %d\n", portnum);
                OWERROR(OWERROR_LIBUSB_SET_CONFIGURATION_ERROR);
		return FALSE;
	}

	/* claim the interface */
	if (usb_claim_interface(usbhnd[portnum], 0) < 0) {
//           printf("owAcquire_DS9490 usb_claim_interface failed\n");
		//strcpy(return_msg, "Failed to claim interface\n");
// 		printf("%s\n", usb_strerror());
          usb_close(usbhnd[portnum]);
          usbhnd[portnum] = NULL ;
                OWERROR(OWERROR_LIBUSB_CLAIM_INTERFACE_ERROR);
		return FALSE;
	}

	/* set the alt interface */
	if (usb_set_altinterface(usbhnd[portnum], 3)) {
//           printf("owAcquire_DS9490 Failed to set altinterface\n");
		//strcpy(return_msg, "Failed to set altinterface\n");
// 		printf("%s\n", usb_strerror());
		usb_release_interface(usbhnd[portnum], 0);
                usb_close(usbhnd[portnum]);
                usbhnd[portnum] = NULL ;
                OWERROR(OWERROR_LIBUSB_SET_ALTINTERFACE_ERROR);
		return FALSE;
	}

	// clear USB endpoints before doing anything with them.
	usb_clear_halt(usbhnd[portnum], DS2490_EP3);
	usb_clear_halt(usbhnd[portnum], DS2490_EP2);
	usb_clear_halt(usbhnd[portnum], DS2490_EP1);

    // verify adapter is working
    if (!AdapterRecover(portnum))
    {
		usb_release_interface(usbhnd[portnum], 0); // release interface
		usb_close(usbhnd[portnum]);   // close handle
		usbhnd[portnum] = NULL ;
        return FALSE;
    }

    // reset the 1-Wire
    owTouchReset_DS9490(portnum);

/* we're all set here! */
	//strcpy(return_msg, "DS2490 successfully acquired by USB driver\n");
   // get a handle to the device
   /*usbhnd[portnum] = CreateFile(port_zstr,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);*/

   //~ if (usbhnd[portnum] != INVALID_HANDLE_VALUE)
   //~ {
      //~ // verify adapter is working
      //~ if (!AdapterRecover(portnum))
      //~ {
         //~ CloseHandle(usbhnd[portnum]);
         //~ return FALSE;
      //~ }
      //~ // reset the 1-Wire
      //~ owTouchReset_DS9490(portnum);
   //~ }
   //~ else
   //~ {
      //~ // could not get resouse
      //~ OWERROR(OWERROR_GET_SYSTEM_RESOURCE_FAILED);
      //~ return FALSE;
   //~ }

  //fd[portnum] = 1 ;
  return TRUE;
}

//---------------------------------------------------------------------------
// Attempt to acquire the specified 1-Wire net.
//
// 'port_zstr'     - zero terminated port name.
//
// Returns: port number or -1 if not successful in setting up the port.
//
int owAcquireEx_DS9490(char *port_zstr)
{
   int portnum = 0;

   /*if(!fd_init)
   {
     int i ;
      for(i=0; i<MAX_PORTNUM; i++)
         fd[i] = 0;
      fd_init = 1;
   }*/
   
   USBInit() ;
   
   if (strcmp(port_zstr, "USB") == 0)
   {
	/* No portnum specified in name - get first available */
	// check to find first available handle slot
	for(portnum = 0; portnum<MAX_PORTNUM; portnum++)
	{
	if((usbhnd[portnum]==NULL) && (usb_dev_list[portnum]!=NULL))
		break;
	}
        if ((portnum>=MAX_PORTNUM)||(portnum<0))
          //printf("owAcquireEx_DS9490 failed to find clear handle\n");
	OWASSERT( portnum<MAX_PORTNUM, OWERROR_PORTNUM_ERROR, -1 );
   }
   else if (sscanf(port_zstr, "USB:%d", &portnum) == 1)
   {
     /* Check that portnum is available */
	if((usbhnd[portnum]!=NULL) || (usb_dev_list[portnum]==NULL))
	{
          /*printf("specific portnum requested not available: %d %s %s\n", 
                 portnum, 
                 (usbhnd[portnum]!=NULL) ? "usbhnd not NULL" :"usbhnd NULL",
          (usb_dev_list[portnum]==NULL) ? "usb_dev_list NULL": "usb_dev_list not NULL");*/
          OWERROR(OWERROR_GET_SYSTEM_RESOURCE_FAILED);
	  return -1 ;
	}
   }
   else /* Bad port name */
   {
     //printf("owAcquireEx_DS9490 bad port name\n");
     OWERROR(OWERROR_PORTNUM_ERROR);
     return -1 ;
   }

   if(!owAcquire_DS9490(portnum, "USB"))
   {
      return -1;
   }

   //~ // get a handle to the device
   //~ usbhnd[portnum] = CreateFile(port_zstr,
                       //~ GENERIC_READ | GENERIC_WRITE,
                       //~ FILE_SHARE_READ,
                       //~ NULL,
                       //~ OPEN_EXISTING,
                       //~ FILE_ATTRIBUTE_NORMAL,
                       //~ NULL);

   //~ if (usbhnd[portnum] != INVALID_HANDLE_VALUE)
   //~ {
      //~ // verify adapter is working
      //~ if (!AdapterRecover(portnum))
      //~ {
         //~ CloseHandle(usbhnd[portnum]);
         //~ return -1;
      //~ }
      //~ // reset the 1-Wire
      //~ owTouchReset_DS9490(portnum);
   //~ }
   //~ else
   //~ {
      //~ // could not get resouse
      //~ OWERROR(OWERROR_GET_SYSTEM_RESOURCE_FAILED);
      //~ return -1;
   //~ }

   return portnum;
}

//---------------------------------------------------------------------------
// Release the port previously acquired a 1-Wire net.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
//
void owRelease_DS9490(int portnum)
{
	usb_release_interface(usbhnd[portnum], 0);
	usb_close(usbhnd[portnum]);
	usbhnd[portnum] = NULL;
  //fd[portnum] = 1 ;
	//strcpy(return_msg, "DS2490 successfully released by USB driver\n");
  // CloseHandle(usbhnd[portnum]);
}
