/* devices.c */

/*
   1-wire devices used by the WS, and their allocations, &C

   Oww project
   Dr. Simon J. Melhuish

   Tue 12th December 2000
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "barom.h"
#include "devices.h"
#include "wstypes.h"
#include "werr.h"
#include "oww.h"
#include "ownet.h"
//#include "mlan.h"
#include "globaldef.h"
#include "setup.h"
#include "lnk.h"
#include "weather.h"
#include "intl.h"
#include "tai8590.h"
#include "thermometer.h"
#include "barometer.h"
#include "sllist.h"
#include "wsi603a.h"

/* Prototypes for branch operations */
#include "swt1f.h"
#include "progstate.h"

#define DEVICES_DBLEV WERR_DEBUG1
#define MAXBRANCHSEARCH 16

# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif

// /* external MLan functions */
// extern void MLanFamilySearchSetup(int); /* OK for branches too */
// extern int MLanFirst(int DoReset, int OnlyAlarmingDevices) ;
// extern int  MLanNext(int,int); /* Trunk only */
// extern void MLanSerialNum(uchar *, int); /* OK for branches too */
// extern int  MLanAccess(void); /* Trunk only */
// extern int  MLanTouchReset(void) ;
// extern int  MLanBlock(int, uchar *, int);
// extern int  Aquire1WireNet(char *, char *);
// extern void Release1WireNet(char *);
//
// extern uchar SerialNum[8] ;

typedef struct {
  int search_entry;
  char *target;
  int wipe;
  void (*callback)(void);
} devalloc;

sllist *realloc_list = NULL;

#define BLANKID "\0\0\0\0\0\0\0"

devices_struct protoVaneSwitch = {BLANKID, {SWITCH_FAMILY, '\0'}, -1, -1, 0, "Vane switch",
    devtype_vane, DefCalib, 0, NULL, NULL, NULL} ;

devices_struct protoVaneAdc = {BLANKID, {ATOD_FAMILY, SBATTERY_FAMILY, '\0'}, -1, -1, 0, "Vane ADC",
    devtype_vane, DefCalib, 0, NULL, NULL, NULL} ;

devices_struct protoVaneAds = {BLANKID, {SBATTERY_FAMILY, '\0'}, -1, -1, 0, "Vane ADS",
    devtype_vane_cal, DefCalib, 1, NULL, NULL, NULL} ;

devices_struct protoVaneIns = {BLANKID, {SBATTERY_FAMILY, '\0'}, -1, -1, 0, "Vane INS",
    devtype_vane_cal, DefCalib, 1, NULL, NULL, NULL} ;

devices_struct protoVaneId = {BLANKID, {DIR_FAMILY, '\0'}, -1,    -1, 0, "Wind vane ",
    devtype_unspec, DefCalib, 0, NULL, NULL, NULL} ;

devices_struct protoAnemometer = {BLANKID, {COUNT_FAMILY, '\0'}, -1,  -1, 0, "Anemometer",
    devtype_anem, DefCalib, 1, NULL, NULL, NULL} ;

devices_struct protoRain = {BLANKID, {COUNT_FAMILY, '\0'}, -1,  -1, 0, "Rain gauge",
    devtype_rain, {RAINSLOPE, 0.0F, 0.0F, 0.0F, 0.0F}, 1, NULL, NULL, NULL} ;

devices_struct protoThermometer = {BLANKID, {TEMP_FAMILY, TEMP_FAMILY_DS1822, TEMP_FAMILY_DS18B20, SBATTERY_FAMILY, '\0'},
    -1,   -1, 0, "Thermometer ",
    devtype_T, DefCalib, 2, NULL, NULL, NULL} ;

devices_struct protoSoilThermometer = {BLANKID, {TEMP_FAMILY, TEMP_FAMILY_DS1822, TEMP_FAMILY_DS18B20, SBATTERY_FAMILY, '\0'},
    -1,   -1, 0, "Soil Thermometer ",
    devtype_T, DefCalib, 2, NULL, NULL, NULL} ;

devices_struct protoIndoorThermometer = {BLANKID, {TEMP_FAMILY, TEMP_FAMILY_DS1822, TEMP_FAMILY_DS18B20, SBATTERY_FAMILY, '\0'},
    -1,   -1, 0, "Indoor Thermometer ",
    devtype_T, DefCalib, 2, NULL, NULL, NULL} ;

devices_struct protoHumidity = {BLANKID, {SBATTERY_FAMILY, HYGROCHRON_FAMILY, '\0'}, -1,   -1, 0, "Humidity ",
    devtype_RH, {1.0F, 0.0F, 1.0F, 0.0F, 0.0F}, 4, NULL, NULL, NULL} ;

devices_struct protoBarometer = {BLANKID, {SBATTERY_FAMILY, SWITCH_FAMILY, '\0'}, -1,   -1, 0, "Barometer ",
    devtype_barom, {Barom_slope, Barom_offset, 1.0F, 0.0F, 0.0F}, 4, NULL, NULL, NULL} ;

devices_struct protoTai8570w = {BLANKID, {SWITCH_FAMILY, '\0'}, -1, -1, 0, "TAI8570 writer ",
    devtype_tai8570w, DefCalib, 0, NULL, NULL, NULL} ;

devices_struct protoSolar = {BLANKID, {SBATTERY_FAMILY, '\0'}, -1,   -1, 0, "Solar ",
    devtype_solar, {SOLAR_SLOPE, SOLAR_OFFSET, 0.0F, 0.0F, 0.0F}, 2, NULL, NULL, NULL} ;

devices_struct protoUV = {BLANKID, {HOBBYBOARDS_FAMILY, '\0'}, -1,   -1, 0, "UV ",
    devtype_uv, {UV_SLOPE, UV_OFFSET, 0.0F, 0.0F, 0.0F}, 2, NULL, NULL, NULL} ;

devices_struct protoADC = {BLANKID, {ADC_DS2760_FAMILY, '\0'}, -1,   -1, 0, "ADC ",
    devtype_adc, {ADC_VSLOPE, ADC_VOFFSET, ADC_ISLOPE, ADC_IOFFSET, ADC_ACCSLOPE}, 5, NULL, NULL, NULL} ;

devices_struct protoTC = {BLANKID, {ADC_DS2760_FAMILY, '\0'}, -1,   -1, 0, "TC ",
    devtype_tc, {TC_TYPE, TC_VOFFSET, 0.0F, 0.0F, 0.0F}, 2, NULL, NULL, NULL} ;

devices_struct protoWSI603A = {BLANKID, {ADC_DS2760_FAMILY, '\0'}, -1,   -1, 0, "WSI603A",
	devtype_wsi603a,
	{
		1.0F /* T slope */, 0.0F /* T offset */,
		1.0F /* Anem slope */,
		1.0F /* Solar slope */, 0.0F /* Solar offset */
	}, 5, NULL, NULL, wsi603a_special_handler} ;

devices_struct protoMoisture = {BLANKID, {HOBBYBOARDS_NO_T_FAMILY, '\0'}, -1,   -1, 0, "Moist ",
    devtype_moisture, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F}, 0, NULL, NULL, hbmoist_parser_special} ;

devices_struct protoLcd = {BLANKID, {PIO_FAMILY, '\0'}, -1,   -1, 0, "LCD ",
    devtype_lcd, DefCalib, 1, NULL, NULL, NULL} ;

devices_struct protoBranch = {BLANKID, {BRANCH_FAMILY, '\0'}, -1,   -1, 0, "Branch ",
    devtype_branch, DefCalib, 0, NULL, NULL, NULL} ;

devices_struct protoGpc = {BLANKID, {COUNT_FAMILY, '\0'}, -1,  -1, 0, "GP Counter ",
    devtype_gpc, {1.0F, 0.0F, 0.0F, 0.0F, 0.0F}, 1, NULL, NULL, NULL} ;

devices_struct *devices_list = NULL ;

extern int wv_known ;

int devices_remote_T[MAXTEMPS] =
{0, 0, 0, 0, 0, 0, 0, 0},
devices_remote_soilT[MAXSOILTEMPS] =
{0, 0},
devices_remote_indoorT[MAXINDOORTEMPS] =
{0, 0},
devices_remote_RH[MAXHUMS] =
{0, 0, 0, 0, 0, 0, 0, 0},
devices_remote_barom[MAXBAROM] =
{0, 0, 0, 0, 0, 0, 0, 0},
devices_remote_gpc[MAXGPC] =
{0, 0, 0, 0, 0, 0, 0, 0},
devices_remote_solar[MAXSOL] =
{0, 0, 0, 0},
devices_remote_uv[MAXUV] =
{0, 0, 0, 0},
devices_remote_adc[MAXADC] =
{0, 0, 0, 0},
devices_remote_soilmoisture[MAXMOIST*4] =
{0, 0, 0, 0, 0, 0, 0, 0},
devices_remote_leafwetness[MAXMOIST*4] =
{0, 0, 0, 0, 0, 0, 0, 0},
devices_remote_tc[MAXTC] =
{0, 0, 0, 0},
devices_remote_vane = 0,
devices_remote_rain = 0,
devices_remote_used = 0 ;

/* The list of search results and their allocations */
devices_search_struct *devices_search_list = NULL ;

/* Number of entries in search list */
int devices_known = 0 ;

/* The current active branch - -1 for trunk */
int devices_active_branch = -1 ;

/* Device locations in list */
int devices_vane      = -1 ;
int devices_vane_adc  = -1 ;
int devices_anem      = -1 ;
int devices_wsi603a   = -1 ;
int devices_vane_ads  = -1 ;
int devices_vane_ins  = -1 ;
int devices_rain      = -1 ;
int devices_T1        = -1 ;
int devices_soilT1    = -1 ;
int devices_indoorT1  = -1 ;
int devices_H1        = -1 ;
int devices_BAR1      = -1 ;
int devices_tai8570w1 = -1 ;
int devices_GPC1      = -1 ;
int devices_sol1      = -1 ;
int devices_uv1       = -1 ;
int devices_adc1      = -1 ;
int devices_tc1       = -1 ;
int devices_lcd1      = -1 ;
int devices_brA       = -1 ;
int devices_wv0       = -1 ;
int devices_moist1    = -1 ;
int DEVICES_TOTAL     =  0 ;


// /* What is the default slope for this device? */
// float
// devices_default_slope(devices_struct *device)
// {
//   switch (device->devtype)
//   {
//     case devtype_barom:
//       return Barom_slope ;
//
//     case devtype_rain:
//       return RAINSLOPE ;
//   }
//
//   return 1.0F ;
// }
//
// /* What is the default offset for this device? */
// float
// devices_default_offset(devices_struct *device)
// {
//   switch (device->devtype)
//   {
//     case devtype_barom:
//       return Barom_offset ;
//   }
//
//   return 0.0F ;
// }

int devices_portnum_var = 0 ;

int devices_portnum(int device)
{
  return devices_portnum_var ;
}

static void
devices_build_from_proto(devices_struct*dev, devices_struct*proto, int index, char *special)
{
  memcpy(dev, proto, sizeof(devices_struct)) ;

  if (special)
  {
    strncat(dev->menu_entry, special, sizeof(dev->menu_entry)-1-strlen(dev->menu_entry)) ;
  }
  else if (index >= 0)
  {
    char iS[8] ;

    sprintf(iS, "%d", index+1) ;
    strncat(dev->menu_entry, iS, sizeof(dev->menu_entry)-1-strlen(dev->menu_entry)) ;
  }
}

/** Add all devices for wind sensor
*
*/
static int
devices_build_wind(devices_struct*dev, int index)
{
  // Add vane switch, 8 vane IDs and a vane adc

  if (dev)
  {
    int i ;
    char * Menu[] = {
      "0 (N)", "1 (NE)", "2 (E)", "3 (SE)",
      "4 (S)", "5 (SW)", "6 (W)", "7 (NW)"
    } ;

    devices_build_from_proto(dev++, &protoVaneSwitch, -1, NULL) ;
    devices_build_from_proto(dev++, &protoVaneAdc, -1, NULL) ;
    devices_build_from_proto(dev++, &protoAnemometer, -1, NULL) ;
    for (i=0; i<8; ++i)
      devices_build_from_proto(dev++, &protoVaneId, i, Menu[i]) ;

    devices_build_from_proto(dev++, &protoWSI603A, -1, NULL) ;
    devices_build_from_proto(dev++, &protoVaneAds, -1, NULL) ;
    devices_build_from_proto(dev++, &protoVaneIns, -1, NULL) ;
  }

  return 14 ;
}

/** Add a thermometer device
*
*/
static int
devices_build_thermometer(devices_struct*dev, int index)
{
  if (dev)
  {
    devices_build_from_proto(dev, &protoThermometer, index, NULL) ;
    thermometer_new(dev) ;
  }

  return 1 ;
}

/** Add a soil thermometer device
*
*/
static int
devices_build_soil_thermometer(devices_struct*dev, int index)
{
  if (dev)
  {
    devices_build_from_proto(dev, &protoSoilThermometer, index, NULL) ;
    thermometer_new(dev) ;
  }

  return 1 ;
}

/** Add an indoor thermometer device
*
*/
static int
devices_build_indoor_thermometer(devices_struct*dev, int index)
{
  if (dev)
  {
    devices_build_from_proto(dev, &protoIndoorThermometer, index, NULL) ;
    thermometer_new(dev) ;
  }

  return 1 ;
}

/** Add a barometer device
*
*/
static int
devices_build_barometer(devices_struct*dev, int index)
{
  if (dev)
  {
    devices_build_from_proto(dev++, &protoBarometer, index, NULL) ;
    barometer_new(dev) ;
  }

  return 1 ;
}

/** Add a barometer TAI8570 write device
*
*/
static int
devices_build_tai8570w(devices_struct*dev, int index)
{
  if (dev)
    devices_build_from_proto(dev, &protoTai8570w, index, NULL) ;

  return 1 ;
}

/** Add a hygrometer device
*
*/
static int
devices_build_hygrometer(devices_struct*dev, int index)
{
  if (dev)
    devices_build_from_proto(dev, &protoHumidity, index, NULL) ;

  return 1 ;
}

/** Add a solar device
*
*/
static int
devices_build_solar(devices_struct*dev, int index)
{
  if (dev)
    devices_build_from_proto(dev, &protoSolar, index, NULL) ;

  return 1 ;
}

/** Add a UV device
*
*/
static int
devices_build_uv(devices_struct*dev, int index)
{
  if (dev)
    devices_build_from_proto(dev, &protoUV, index, NULL) ;

  return 1 ;
}

/** Add an ADC device
*
*/
static int
devices_build_adc(devices_struct*dev, int index)
{
  if (dev)
    devices_build_from_proto(dev, &protoADC, index, NULL) ;

  return 1 ;
}

/** Add a thermocouple device
*
*/
static int
devices_build_thrmcpl(devices_struct*dev, int index)
{
  if (dev)
    devices_build_from_proto(dev, &protoTC, index, NULL) ;

  return 1 ;
}

/** Add a rain gauge
*
*/
static int
devices_build_rain(devices_struct*dev, int index)
{
  if (dev)
    devices_build_from_proto(dev, &protoRain, index, NULL) ;

  return 1 ;
}

/** Add a general purpose counter
*
*/
static int
devices_build_gpc(devices_struct*dev, int index)
{
  if (dev)
    devices_build_from_proto(dev, &protoGpc, index, NULL) ;

  return 1 ;
}

/** Add a moisture device
*
*/
static int
devices_build_moisture(devices_struct*dev, int index)
{
  if (dev) {
    devices_build_from_proto(dev, &protoMoisture, index, NULL) ;
    dev->local = calloc(1, sizeof(moist_struct));
  }

  return 1 ;
}

/** Add an LCD
*
*/
static int
devices_build_lcd(devices_struct*dev, int index)
{
  if (dev)
  {
    devices_build_from_proto(dev, &protoLcd, index, NULL) ;
    tai8590_New(dev) ;
  }

  return 1 ;
}

/** Add a branch
*
*/
static int
devices_build_branch(devices_struct*dev, int index)
{
  if (dev)
  {
    if ((index >= 0) && (index < 26))
    {
      char special[] = "A" ;
      special[0] += (char) index ;
      devices_build_from_proto(dev, &protoBranch, index, special) ;
    }
  }
  return 1 ;
}

int
devices_build_devices_list(void)
{
  // Calc size of devices_list ;

  int membs = 0, i = 0, j ;

  // Call devices_build_ functions with NULL pointer to get sizes to allocate
  membs += devices_build_wind(NULL, 0) ;
  membs += devices_build_rain(NULL, 0) ;
  membs += devices_build_thermometer(NULL, 0) * MAXTEMPS ;
  membs += devices_build_soil_thermometer(NULL, 0) * MAXSOILTEMPS ;
  membs += devices_build_indoor_thermometer(NULL, 0) * MAXINDOORTEMPS ;
  membs += devices_build_hygrometer(NULL, 0) * MAXHUMS ;
  membs += devices_build_barometer(NULL, 0) * MAXBAROM ;
  membs += devices_build_tai8570w(NULL, 0) * MAXBAROM ;
  membs += devices_build_gpc(NULL, 0) * MAXGPC ;
  membs += devices_build_solar(NULL, 0) * MAXSOL ;
  membs += devices_build_uv(NULL, 0) * MAXUV ;
  membs += devices_build_adc(NULL, 0) * MAXADC ;
  membs += devices_build_thrmcpl(NULL, 0) * MAXTC ;
  membs += devices_build_moisture(NULL, 0) * MAXMOIST ;
  membs += devices_build_lcd(NULL, 0) * MAXLCD ;
  membs += devices_build_branch(NULL, 0) * MAXBRANCHES ;

  // Now that we know the space required, allocate memory to devices_list
  devices_list = (devices_struct *) calloc(membs, sizeof(devices_struct)) ;

  if (devices_list == NULL) return -1 ; // Failed

  // Now call the devices_build_ functions again to build the devices for real
  devices_vane = 0 ;
  devices_vane_adc = 1 ;
  devices_anem = 2 ;
  devices_wv0 = 3 ;
  devices_wsi603a = 11 ;
  devices_vane_ads = 12 ;
  devices_vane_ins = 13 ;
  i += devices_build_wind(&devices_list[0], 0) ;

  devices_rain = i ;
  i += devices_build_rain(&devices_list[i], 0) ;

  if (MAXTEMPS > 0)
  {
    devices_T1 = i ;
    for (j=0; j<MAXTEMPS; ++j)
      i += devices_build_thermometer(&devices_list[i], j) ;

    //devices_list[devices_T1].required = 1;
  }

  if (MAXSOILTEMPS > 0)
  {
    devices_soilT1 = i ;
    for (j=0; j<MAXSOILTEMPS; ++j)
      i += devices_build_soil_thermometer(&devices_list[i], j) ;
  }

  if (MAXINDOORTEMPS > 0)
  {
    devices_indoorT1 = i ;
    for (j=0; j<MAXINDOORTEMPS; ++j)
      i += devices_build_indoor_thermometer(&devices_list[i], j) ;
  }

  if (MAXHUMS > 0)
  {
    devices_H1 = i ;
    for (j=0; j<MAXHUMS; ++j)
      i += devices_build_hygrometer(&devices_list[i], j) ;
  }

  if (MAXBAROM > 0)
  {
    devices_BAR1 = i ;
    for (j=0; j<MAXBAROM; ++j)
      i += devices_build_barometer(&devices_list[i], j) ;
  }

  if (MAXBAROM > 0)
  {
    devices_tai8570w1 = i ;
    for (j=0; j<MAXBAROM; ++j)
      i += devices_build_tai8570w(&devices_list[i], j) ;
  }

  if (MAXGPC > 0)
  {
    devices_GPC1 = i ;
    for (j=0; j<MAXGPC; ++j)
      i += devices_build_gpc(&devices_list[i], j) ;
  }

  if (MAXSOL > 0)
  {
    devices_sol1 = i ;
    for (j=0; j<MAXSOL; ++j)
      i += devices_build_solar(&devices_list[i], j) ;
  }

  if (MAXUV > 0)
  {
    devices_uv1 = i ;
    for (j=0; j<MAXUV; ++j)
      i += devices_build_uv(&devices_list[i], j) ;
  }

  if (MAXADC > 0)
  {
    devices_adc1 = i ;
    for (j=0; j<MAXADC; ++j)
      i += devices_build_adc(&devices_list[i], j) ;
  }

  if (MAXTC > 0)
  {
    devices_tc1 = i ;
    for (j=0; j<MAXTC; ++j)
      i += devices_build_thrmcpl(&devices_list[i], j) ;
  }

  if (MAXMOIST > 0)
  {
    devices_moist1 = i ;
    for (j=0; j<MAXMOIST; ++j)
      i += devices_build_moisture(&devices_list[i], j) ;
  }

  if (MAXLCD > 0)
  {
    devices_lcd1 = i ;
    for (j=0; j<MAXLCD; ++j)
      i += devices_build_lcd(&devices_list[i], j) ;
  }

  if (MAXBRANCHES > 0)
  {
    devices_brA = i ;
    for (j=0; j<MAXBRANCHES; ++j)
      i += devices_build_branch(&devices_list[i], j) ;
  }

  DEVICES_TOTAL = i ;

  return DEVICES_TOTAL ; // Ok - Return total number of devices
}

// int devices_id_to_string(char *string, setupp_liststr *member, int index)
// {
//   int devnum ;
//
//   devnum = *((int *)(member->data)) ;
//   if (devnum < 0)
//     werr(1, "devices_id_to_string with -ve devnum") ;
//
//   return setup_id_to_string(string, devices_list[devnum+index].id) ;
// }
//
// int devices_string_to_id(char *string, setupp_liststr *member, int index)
// {
//   int devnum ;
//
//   devnum = *((int *)(member->data)) ;
//   if (devnum < 0)
//     werr(1, "devices_string_to_id with -ve devnum") ;
//
//   return setup_string_to_id(string, devices_list[devnum+index].id) ;
// }

void devices_check_vane_ids(void)
{
  /* Check these anyway, whether or not we have a 2450 sensor */
  for (wv_known=0; wv_known<8; ++wv_known)
  {
    if (devices_list[devices_wv0+wv_known].id[0] != DIR_FAMILY)
      break ;
  }
}

void
devices_wipe_vanes(void)
{
  int i ;

  /* Loop over vane devices */
  for (i=devices_wv0; i<devices_wv0+8; ++i)
  {
    int slnum ;

    slnum = devices_list[i].back_alloc ;
    if (-1 != slnum)
      devices_remove_search_entry(slnum, 0) ;

    devices_list[i].id[0] = '\0' ;
  }

  wv_known = 0 ;
}

int devices_portnum_default(void)
{
  return devices_portnum_var ;
}

/* Clear out the search list - free memory, &c */
int
devices_clear_search_list(void)
{
  int i ;

  /* Free old memory */
  if (devices_search_list)
  {
    free((void *) devices_search_list) ;
    devices_search_list = NULL ;
  }

  devices_known = 0 ;

  /* Remove back allocations from devices_list */
  for (i=0; i<DEVICES_TOTAL; ++i)
  {
    devices_list[i].back_alloc = -1 ;
  }

  return 0 ; /* OK */
}

/* Text for cal menu */

char *
devices_cal_name(int devtype, int i)
{
  switch (devtype)
  {
  case devtype_T:
  {
	switch (i)
	{
	case 0:
	  return _("Thermometer slope") ;

	case 1:
	  return _("Thermometer offset (" DEGSYMB "C)") ;

	default:
	  return NULL ;
	}
	break;
  }

  case devtype_anem:
  {
	if (0 == i) return _("Anemometer slope") ;
	return NULL ;
  }

  case devtype_vane_cal:
  {
	if (0 == i) return _("Compass Offset") ;
	return NULL ;
  }

  case devtype_rain:
  {
	if (0 == i) return _("Rain gauge slope (\"/tip)") ;
	return NULL ;
  }

  case devtype_RH:
  {
	switch (i)
	{
	case 0:
	  return _("Trh slope") ;

	case 1:
	  return _("Trh offset (" DEGSYMB "C)") ;

	case 2:
	  return _("Hygrometer slope") ;

	case 3:
	  return _("Hygrometer offset (%RH)") ;

	default:
	  return NULL ;
	}
	break;
  }

  case devtype_barom:
  {
	switch (i)
	{
	case 0:
	  return _("Barometer slope (mBar / X)") ;

	case 1:
	  return _("Barometer offset (mBar)") ;

	case 2:
	  return _("Tb slope") ;

	case 3:
	  return _("Tb offset (" DEGSYMB "C)") ;

	default:
	  return NULL ;
	}
	break;
  }

  case devtype_gpc:
  {
	if (0 == i) return _("GPC slope") ;
	return NULL ;
  }

  case devtype_solar:
  {
	switch (i)
	{
	case 0:
	  return _("Solar slope") ;

	case 1:
	  return _("Solar offset") ;
	}
	break;
  }

  case devtype_uv:
  {
	switch (i)
	{
	case 0:
	  return _("UVI slope") ;

	case 1:
	  return _("UVI offset") ;
	}
	break;
  }

  case devtype_adc:
  {
	switch (i)
	{
	case 0:
	  return _("Vdd slope");

	case 1:
	  return _("Vdd offset");

	case 2:
	  return _("Vsns slope");

	case 3:
	  return _("Vsns offset");

	case 4:
	  return _("Acc slope");
	}
	break;
  }

  case devtype_tc:
  {
	switch (i)
	{
	case 0:
	  return _("Thermocouple Type");

	case 1:
	  return _("Thermocouple V-off");
	}
	break;
  }

  case devtype_lcd:
  {
	if (0 == i) return _("LCD type") ;
	return NULL ;
  }

  case devtype_wsi603a:
  {
	switch (i)
	{
	case 0:
	  return _("Thermometer slope") ;

	case 1:
	  return _("Thermometer offset (" DEGSYMB "C)") ;

	case 2:
	  return _("Anemometer slope") ;

	case 3:
	  return _("Solar slope") ;

	case 4:
	  return _("Solar offset") ;

	default:
	  return NULL ;
	}
	break;
  }

  default:
  {
	return NULL ;
  }
  }

  return NULL ;
}

/* Clear IDs of device list entries with no back alloc */

int
devices_purge_devices_list(void)
{
  int i ;

  for (i=0; i<DEVICES_TOTAL; ++i)
  {
    if ((devices_list[i].back_alloc < 0) &&
        (devices_list[i].id[0] != 0))
    {
      werr(WERR_DEBUG1, "Purge %s", devices_list[i].menu_entry) ;
      devices_list[i].id[0] = 0 ;
    }
  }

  return 0 ; /* OK */
}



static int
devices_ensure_search_memory(int new_size)
{
  devices_search_list =
    (devices_search_struct *) realloc((void *) devices_search_list,
    sizeof(devices_search_struct) * new_size) ;

  if (!devices_search_list && (new_size>0))
  {
    werr(0, "devices_ensure_search_memory: Out of memory (%d bytes requested)", new_size) ;
    devices_known = 0 ;
    return -1 ;
  }

  return 0 ; /* OK */
}

/* Copy one ID array to another */
int
devices_copy_ids(unsigned char to_id[], unsigned char from_id[])
{
  int i ;

  for (i=0; i<8; ++i)
    to_id[i] = from_id[i] ;

  return 1 ;
}

/* Compare the IDs of two devices 1 => equal */
int
devices_compare_ids(unsigned char id1[], unsigned char id2[])
{
  return
    (0 == memcmp((void *) id1, (void *) id2, 8 * sizeof(unsigned char))) ;
}

/* Allocate 'from' in search list to 'to' in device list */
int
devices_allocate(int to, int from)
{
  werr(DEVICES_DBLEV, "devices_allocate(%d, %d)", to, from) ;
  if (devices_match_family(&devices_list[to],
                            devices_search_list[from].id[0]))
  {
    /* This one seems to be suitable */
    devices_copy_ids(devices_list[to].id, devices_search_list[from].id) ;
    devices_list[to].back_alloc = from ;
    devices_search_list[from].alloc = to ;
    devices_list[to].branch = devices_search_list[from].branch ;
  }
  else
  {
    werr(DEVICES_DBLEV, "devices_allocate requested for wrong family") ;
    return 1 ;
  }

  return 0 ;
}

/* Scan the device list to make a suitable allocation */
int
devices_auto_allocate(int num)
{
  int i ;

  // Check whether autoalloc is enabled (on by default)
  if (setup_autoalloc == 0) return 0 ;

  werr(DEVICES_DBLEV, "devices_auto_allocate(%d)", num) ;

  // Skip the vane devices
  for (i=devices_anem; i<DEVICES_TOTAL; ++i)
  {
    /* Skip if already allocated */
    if (-1 != devices_list[i].back_alloc) continue ;

    /* Skip if not the right family */
    if (!devices_match_family(&devices_list[i],
                            devices_search_list[num].id[0]))
       continue ;
    //if (devices_search_list[num].id[0] != devices_list[i].family) continue ;

    /* Family matches - do allocation */
    devices_allocate(i, num) ;

    werr(DEVICES_DBLEV, "Allocating search entry %d to \"%s\"",
      num, devices_list[i].menu_entry) ;

    return 0 ;
  }

  werr(DEVICES_DBLEV,
       "No suitable allocation for search entry %d, family 0x%02x",
       num,
       devices_search_list[num].id[0]) ;

  return 0 ;
}

/* Remove an entry (num) from the search list */
/* wipe_id == 1 => blank out corresponding ID in device list */
int
devices_remove_search_entry(int num, int wipe_id)
{
  int i ;

  /* Does this have an allocation to devices_list? */
  if (-1 != devices_search_list[num].alloc)
  {
    /* Should we wipe the ID as well? */
    if (wipe_id)
    {
      devices_list[devices_search_list[num].alloc].id[0] = 0 ;
    }

    /* Undo the allocation */
    devices_list[devices_search_list[num].alloc].back_alloc = -1 ;
  }

  /* Are there entries above this one? */
  if (devices_known > num + 1)
  {
    /* We need to move the higher entries down the list */
    /* Loop over these entries */
    for (i=num+1; i<devices_known; ++i)
    {
      /* Decrement any back allocation */
      if (devices_search_list[i].alloc != -1)
      {
        if (devices_list[devices_search_list[i].alloc].back_alloc < 0)
          werr(DEVICES_DBLEV, "devices_remove_search_entry found bad back_alloc") ;
        --devices_list[devices_search_list[i].alloc].back_alloc ;
      }

      /* Copy down the list */
      devices_search_list[i-1] = devices_search_list[i] ;
    }
  }

  --devices_known ;

  if (-1 == devices_ensure_search_memory(devices_known))
    return -1 ;

  return 0 ; /* OK */
}

/* Branch name */
static char *
devices_branch_name(int branch)
{
  static char result[64] ;

  if (branch >= devices_brA)
  {
    sprintf(result,
            "%s [%s]",
            devices_list[DevicesBranchNum(branch)].menu_entry,
            (DevicesMainOrAux(branch)) ?
            "main" :
            "aux") ;

    return result ;
  }

  return "Trunk" ;
}

/* Expand search list, add an ID and check allocations */
int
devices_new_search_entry(unsigned char id[],
                         int branch_num,
                         int do_allocation)
{
  int i, new_entry = -1 ;

  /* First scan the search list for this entry in case it's already on */
  for (i=0; i<devices_known; ++i)
  {
    if (devices_compare_ids(devices_search_list[i].id, id))
    {
      /* Yes - this ID is already on the search list */
      new_entry = i ;
      break ;
    }
  }

  if (new_entry == -1)
  {
    /* Need a fresh entry */
    new_entry = devices_known ;

    if (-1 == devices_ensure_search_memory(1+devices_known))
      return -1 ;

    devices_copy_ids(devices_search_list[new_entry].id, id) ;
    ++devices_known ;
  }

  /* Assume this device is not in use */
  devices_search_list[new_entry].alloc = -1 ;

  /* Copy branch number */
  devices_search_list[new_entry].branch = branch_num ;
  werr(DEVICES_DBLEV,
       "search entry %d is on %s",
       new_entry,
       devices_branch_name(branch_num)) ;

  /* Check forward and backward allocations */

  for (i=0; i<DEVICES_TOTAL; ++i)
  {
    char idstring1[24], idstring2[24] ;
    setup_id_to_string(idstring1, devices_list[i].id) ;
    setup_id_to_string(idstring2, devices_search_list[new_entry].id) ;
    werr(WERR_DEBUG2,
      "Comparing search id \"%s\" with device list entry \"%s\" (back_alloc = %d)",
      idstring2, idstring1, devices_list[i].back_alloc) ;
    if (devices_compare_ids(devices_list[i].id,
        devices_search_list[new_entry].id))
    {
      /* The IDs match - the new ID is being used by element i */
      devices_list[i].back_alloc = new_entry ;
      devices_search_list[new_entry].alloc = i ;
      werr(DEVICES_DBLEV, "Search item %d was previously stored as \"%s\"",
        new_entry, devices_list[i].menu_entry) ;
      break ;
    }
  }

  if (-1 == devices_search_list[new_entry].alloc)
  {
    /* This ID was not found in devices_list */
    werr(DEVICES_DBLEV, "Search item %d not allocated yet", new_entry) ;

    /* Now allocate it */
    if (do_allocation)
      devices_auto_allocate(new_entry) ;
  }

  return 0 ; /* OK */
}

int
devices_search_and_add_branch(unsigned char family,
                              int do_allocation,
                              int branch_num,
                              int main_or_aux)
{
  uchar BranchSN[MAXBRANCHSEARCH][8] ;
  uchar extra[1];
  int i, found ;

  memset((void *) BranchSN,
         0,
         MAXBRANCHSEARCH * 8 * sizeof(uchar)) ;

  /*MLanFamilySearchSetup(family) ;*/

  found = FindBranchDevice(devices_portnum_default(),
                           devices_list[branch_num].id,
                           BranchSN,
                           MAXBRANCHSEARCH,
                           main_or_aux) ;

  if (!SetSwitch1F(devices_portnum_default(), devices_list[branch_num].id, 0, 0, extra, 1))
    werr(WERR_DEBUG0, "SetSwitch1F all lines off failed") ;

  if (!found)
  {
    werr(DEVICES_DBLEV, "None found") ;
    return 0 ; /* None found, but OK */
  }

  /* Some were found */

  for (i=0; i<found; ++i)
  {
    char id_string[20] ;

    setup_id_to_string(id_string, BranchSN[i]) ;
    werr(DEVICES_DBLEV, "Found %s", id_string) ;

    if (family && (BranchSN[i][0] != family))
    {
      char idstring[32] ;

      /* Shouldn't have happened? */
      werr(DEVICES_DBLEV /*DEVICES_DBLEV*/,
        "devices_search_and_add_branch found family %x instead of %x",
        (int) BranchSN[i][0], (int) family) ;
      setup_id_to_string(idstring, BranchSN[i]) ;
      werr(DEVICES_DBLEV /*DEVICES_DBLEV*/,
           "ID is %s",
           idstring) ;
    }
    else
    {
      /* success! correct device type found */
      /* Add to list of search results */
      devices_new_search_entry(BranchSN[i],
                               DevicesBranchCode(branch_num, main_or_aux),
                               do_allocation) ;
      werr(DEVICES_DBLEV,
           "Found device on %s",
           devices_list[branch_num].menu_entry) ;
    }
  }

  /* Done */

  return 0 ;
}

/* Search for 1-wire devices from family and add to search list */
/* family == 0 => look for all devices */

int
devices_search_and_add(unsigned char family, int do_allocation)
{
  unsigned char search_sn[12] ;
  char id_string[20] ;
  int br ;
// --- #dwmatt - MOD_HBHUB
//  int hb ;
// ---
  int count=0;

  werr(DEVICES_DBLEV,
    "devices_search_and_add(0x%x, %d)", (int) family, do_allocation) ;

  //MLanTouchReset() ;

  /* Want to search for family */
  owFamilySearchSetup(devices_portnum_var, family) ;

  /* Search for all matches */
  while((count<DEVICES_MAX_SEARCH) && (owNext(devices_portnum_var, TRUE,FALSE)))
  {
	++count;

    /* Check the serial number */
    owSerialNum(devices_portnum_default(), search_sn,TRUE) ;
    setup_id_to_string(id_string, search_sn) ;
    werr(DEVICES_DBLEV,
      "Found device %s (looking for %x)", id_string, family) ;

    /* Is it the correct family? */
    if ((0 != family) && ((0x7f & search_sn[0]) != (0x7f & family)))
    {
      /* Shouldn't have happened? */
      werr(DEVICES_DBLEV,
        "devices_search_and_add found family %x instead of %x",
        (int) search_sn[0], (int) family) ;
      //MLanSkipFamily() ;
      //break ;
    }
    else
    {
      /* success! correct device type found */
      /* Add to list of search results */

      devices_new_search_entry(search_sn, -1, do_allocation) ;
      werr(DEVICES_DBLEV,
        "Stored %s on search list", id_string) ;
    }
  }

  /* Now check each of the branches */

  for (br=devices_brA; br<devices_brA+MAXBRANCHES; ++br)
  {
    int main_or_aux ;

    /* Check both main and aux */

    if (devices_list[br].id[0] == BRANCH_FAMILY)
    {
      werr(DEVICES_DBLEV,
           "Looking down branch %s for family 0x%x",
           devices_list[br].menu_entry,
           family) ;
      for (main_or_aux=0; main_or_aux<2; ++main_or_aux)
      {
        werr(DEVICES_DBLEV,
             "main_or_aux = %d",
             main_or_aux) ;
        if (devices_search_and_add_branch(family,
                                          do_allocation,
                                          br,
                                          main_or_aux))
          return 1 ;
      }
    }
  }

// --- #dwmatt - MOD_HBHUB
  // Now search each port of each HB Hub
//  for (hb=devices_HBHub; hb<devices_HBHub+MAXBRANCHES; ++hb)
//  {
//    int portnum ;		// We normally have 4 of these
//
//    if (devices_list[hb].id[0] == HOBBYBOARDS_NO_T_FAMILY)
//    {
//      werr(DEVICES_DBLEV,
//           "Looking down port %s for family 0x%x",
//           devices_list[hb].menu_entry,
//           family) ;
//      for (portnum=0; portnum<4; portnum++)
//      {
//        werr(DEVICES_DBLEV,
//             "portnum = %d",
//             portnum) ;
//        
// Actually do the search here...
//
//          return 1 ;
//      }
//    }
//  }
// ---


  werr(DEVICES_DBLEV, "End of devices_search_and_add") ;
  return 0 ; /* OK */
}

/* In normal use we get devices_list from devices file */
/* Build a fake search list from this information */
/* add_all => cover whole list, not just wv* */
int
devices_build_search_list_from_devices(int add_all)
{
  int i ;

  /* Loop over entries in devices_list */
  for (i= ((add_all)? 0 : devices_wv0);
       i< ((add_all)? DEVICES_TOTAL : devices_wv0+8);
       ++i)
  {
    if (devices_list[i].id[0] != '\0')
    {
      werr(DEVICES_DBLEV, "Adding \"%s\" to fake search list",
        devices_list[i].menu_entry) ;

      /* Add to list - don't auto-allocate */
      devices_new_search_entry(/* id */ devices_list[i].id,
                               /* no branch */ -1,
                               /* don't allocate */ 0) ;

      /* Manual allocation */
      devices_allocate(i, devices_known-1) ;
    }
  }

  werr(DEVICES_DBLEV, "Summary");
  for (i=0;i<DEVICES_TOTAL;++i)
    if (werr_will_output(DEVICES_DBLEV))
        werr(DEVICES_DBLEV, "%s(%d) > search list %d%s",
        devices_list[i].menu_entry,
        i,
        devices_list[i].back_alloc,
        ((devices_list[i].branch >= devices_brA) &&
         (devices_list[i].branch < devices_wv0)) ?
        devices_list[devices_list[i].branch].menu_entry :
        "") ;


  return 0 ; /* OK */
}

/* Find a devices_list entry by name */
static int
devices_find_by_name(char *name)
{
  int i ;

  /* Loop over all devices_list entries */
  for (i=0; i<DEVICES_TOTAL; ++i)
  {
    if (0 == strcmp(devices_list[i].menu_entry, name)) return i ;
  }

  /* If execution reaches here, name not found */
  return -1 ;
}

/* Swap the calibration values */
static int
devices_swap_calib(int dev1, int dev2)
{
  void *cal ;

  /* Is it legal to swap calib between these devices? */
  if ((devices_list[dev1].devtype !=
       devices_list[dev2].devtype) ||
      (devices_list[dev1].ncalib !=
       devices_list[dev2].ncalib))
    return -1 ; /* Illegal swap */

  cal = malloc(devices_list[dev1].ncalib * sizeof(float)) ;
  if (!cal) return -1 ; /* Error */

  memcpy(cal,
         devices_list[dev1].calib,
         devices_list[dev1].ncalib * sizeof(float)) ;

  memcpy(devices_list[dev1].calib,
         devices_list[dev2].calib,
         devices_list[dev1].ncalib * sizeof(float)) ;

  memcpy(devices_list[dev2].calib,
         cal,
         devices_list[dev1].ncalib * sizeof(float)) ;

  free(cal) ;

  return 0 ; /* Ok */
}

/* Swap the local data */
static int
devices_swap_local(int dev1, int dev2)
{
  void * temp ;

  werr(DEVICES_DBLEV, "swap local %s -> %s in state %s\n",
	  devices_list[dev1].menu_entry, devices_list[dev2].menu_entry,
	  state_get_name(prog_state));

  /* Is it legal to swap local data between these devices? */
  if (devices_list[dev1].devtype != devices_list[dev2].devtype)
    return -1 ; /* Illegal swap */

  devices_swap_calib(dev1, dev2);

  temp = devices_list[dev1].local ;
  devices_list[dev1].local = devices_list[dev2].local ;

  devices_list[dev2].local = temp ;

  return 0 ; /* Ok */
}

/* Allocate a search list entry by name, swapping if necessary */
/* search_entry - index into devices_search_list */
/* target       - name of target in devices_list */
/* wipe         - do we clear target ID for --None-- ? */
static int devices_reallocate(int search_entry, char *target, int wipe)
{
  int target_old_back_alloc, target_i, current_alloc ;

  current_alloc = devices_search_list[search_entry].alloc ;

  /* Check for special cases */
  if ((!target) || (0 == strcmp(_("--None--"), target)))
  {
//    werr(DEVICES_DBLEV, "Deallocate %d", search_entry) ;
    /* Just deallocate */
    if (current_alloc >= 0)
    {
//      werr(DEVICES_DBLEV, "Previously allocated to \"%s\"",
//        devices_list[current_alloc].menu_entry) ;
      devices_list[current_alloc].back_alloc = -1 ;
      if (wipe)
      {
        /* Flag out the id */
        devices_list[current_alloc].id[0] = '\0' ;
      }
      devices_search_list[search_entry].alloc = -1 ;
    }
    return 0 ; /* OK */
  }

  /* Look up the name */
  target_i = devices_find_by_name(target) ;

  if (target_i < 0) return -1 ; /* Failure - not found */

  target_old_back_alloc = devices_list[target_i].back_alloc ;

  if (target_old_back_alloc >= 0)
  {
    /* There was an old allocation to the target */
    /* Change it to current_alloc */
    /* Do we have an old alloc to swap for it? */
    if (current_alloc >= 0)
    {
      /* Yes - we have an old alloc to swap */
      devices_allocate(current_alloc, target_old_back_alloc) ;

      /* Swap the local data */
      devices_swap_local(current_alloc, target_i) ;

//      werr(DEVICES_DBLEV, "\"%s\" now device %d",
//        devices_list[current_alloc].menu_entry,
//        target_old_back_alloc) ;

    }
    else
    {
      /* We have no current allocation - deallocate old alloc */

//      werr(DEVICES_DBLEV, "Deallocating search entry %d",
//        target_old_back_alloc) ;
      devices_search_list[target_old_back_alloc].alloc = -1 ;
    }
  }
  else
  {
    /* No search list entry was allocated to this new device
       so we should just clear the old device and proceed with
       allocation */

    if (current_alloc >= 0)
    {
      devices_list[current_alloc].back_alloc = -1 ;
      if (wipe)
        devices_list[current_alloc].id[0] = '\0' ;
      devices_search_list[search_entry].alloc = -1 ;

      /* Swap the local data */
      devices_swap_local(current_alloc, target_i) ;
    }
  }

  devices_allocate(target_i, search_entry) ;

  werr(DEVICES_DBLEV, "\"%s\" now device %d",
    devices_list[target_i].menu_entry,
    search_entry) ;

  return 0 ; /* OK */
}

static int devices_check_realloc(void)
{
  while (realloc_list != NULL)
  {
	devalloc *ap;
	ap = realloc_list->data;
	devices_reallocate(ap->search_entry, ap->target, ap->wipe);
	if (ap->callback != NULL) ap->callback();
//	devices_swap_local(ap->dev1, ap->dev2);
	free(ap->target);
	free(ap);
	realloc_list = sllist_delete(realloc_list, realloc_list);
  }
  return 0; // Ok
}

int devices_queue_realloc(int search_entry, char *target, int wipe, void (*callback)(void))
{
  devalloc *ap;
  ap = calloc(1, sizeof(devalloc));
  if (ap==NULL) return -1;
  ap->search_entry = search_entry;
  ap->target = (target==NULL) ? NULL : strdup(target);
  ap->wipe = wipe;
  ap->callback = callback;
  sllist_append(&realloc_list, ap);
  return 0;
}

/* Return a string corresponding to the family type */
char *
devices_get_type(unsigned char family)
{
  switch (family)
  {
    case BRANCH_FAMILY:
      return _("Coupler") ;

    case TEMP_FAMILY:
      return _("Thermometer") ;

    case SWITCH_FAMILY:
      return _("Switch") ;

    case COUNT_FAMILY:
      return _("Counter") ;

    case DIR_FAMILY:
      return _("ROM ID") ;

    case ATOD_FAMILY:
      return _("A to D") ;

    case SBATTERY_FAMILY:
      return _("Smart Bat' Mon'") ;

    case ADC_DS2760_FAMILY:
      return _("DS2760 Bat' Mon'");

    case HOBBYBOARDS_FAMILY:
      return _("Hobbyboards");

    case HOBBYBOARDS_NO_T_FAMILY:
      return _("Hobbyboards no T");

    case PIO_FAMILY:
      return _("8-bit IO") ;

    case HYGROCHRON_FAMILY:
      return _("Hygrochron") ;

    case NULL_FAMILY:
      return "" ;

    /* Other families we don't use */
    case 0x02:
      return _("MultiKey") ;

    case 0x04:
    case 0x06:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0F:
    case 0x23:
    case 0x89:
    case 0x8B:
    case 0x8F:
      return _("Memory") ;

    case 0x05:
      return _("DS2405 Switch") ;

    case 0x08:
      return _("Mem/Clk") ;

    case 0x14:
      return _("EEPROM") ;

    case 0x18:
      return _("SHA") ;

    case 0x1A:
      return _("Monetary") ;

    case 0x1B:
      return _("DS2436") ;

    case 0x1C:
      return _("DS2432") ;

    case 0x1E:
      return _("DS2437") ;

    case 0x21:
      return _("Thermochron") ;

    case TEMP_FAMILY_DS1822:
      return _("DS1822") ;

    case TEMP_FAMILY_DS18B20:
      return _("DS18B20") ;

    case 0x81:
      return _("Serial") ;

    case 0x82:
      return _("Multi") ;

    case 0x84:
      return _("Time") ;
  }

  return "----" ;
}

/* Do we have thermometer i? */
int
devices_have_thermometer(int i)
{
  if (devices_remote_used)
  {
	if (i<MAXTEMPS)
      return devices_remote_T[i] ;
	i -= MAXTEMPS;
	if (i<MAXSOILTEMPS)
	  return devices_remote_soilT[i] ;
	i -= MAXSOILTEMPS;
	if (i<MAXINDOORTEMPS)
	  return devices_remote_indoorT[i];
	return FALSE;
  }

  return (
    (devices_list[devices_T1+i].id[0] == TEMP_FAMILY) ||
    (devices_list[devices_T1+i].id[0] == TEMP_FAMILY_DS1822) ||
    (devices_list[devices_T1+i].id[0] == TEMP_FAMILY_DS18B20) ||
    (devices_list[devices_T1+i].id[0] == SBATTERY_FAMILY)) ;
}

/* Do we have thermometer data i? */
int
devices_have_temperature(int i)
{
  if (devices_remote_used)
    return devices_remote_T[i] ;

  /* WSI603A provides T1 by default */
  if ((i==0) && devices_have_wsi603a()) return 1;

  return (
    (devices_list[devices_T1+i].id[0] == TEMP_FAMILY) ||
    (devices_list[devices_T1+i].id[0] == TEMP_FAMILY_DS1822) ||
    (devices_list[devices_T1+i].id[0] == TEMP_FAMILY_DS18B20) ||
    (devices_list[devices_T1+i].id[0] == SBATTERY_FAMILY)) ;
}

/* Do we have soil thermometer data i? */
int
devices_have_soil_temperature(int i)
{
  if (devices_remote_used)
    return devices_remote_soilT[i] ;

  return (
    (devices_list[devices_soilT1+i].id[0] == TEMP_FAMILY) ||
    (devices_list[devices_soilT1+i].id[0] == TEMP_FAMILY_DS1822) ||
    (devices_list[devices_soilT1+i].id[0] == TEMP_FAMILY_DS18B20) ||
    (devices_list[devices_soilT1+i].id[0] == SBATTERY_FAMILY)) ;
}

/* Do we have indoor thermometer data i? */
int
devices_have_indoor_temperature(int i)
{
  if (devices_remote_used)
    return devices_remote_indoorT[i] ;

  return (
    (devices_list[devices_indoorT1+i].id[0] == TEMP_FAMILY) ||
    (devices_list[devices_indoorT1+i].id[0] == TEMP_FAMILY_DS1822) ||
    (devices_list[devices_indoorT1+i].id[0] == TEMP_FAMILY_DS18B20) ||
    (devices_list[devices_indoorT1+i].id[0] == SBATTERY_FAMILY)) ;
}

/* Do we have humidity sensor i? */
int
devices_have_hum(int i)
{
  if (devices_remote_used)
    return devices_remote_RH[i] ;

  return ((devices_list[devices_H1+i].id[0] == SBATTERY_FAMILY) ||
          (devices_list[devices_H1+i].id[0] == HYGROCHRON_FAMILY)) ;
}

int
devices_have_barom(int i)
{
  if (devices_remote_used)
    return devices_remote_barom[i] ;

  return ((devices_list[devices_BAR1+i].id[0] == SBATTERY_FAMILY) ||
         ((devices_list[devices_BAR1+i].id[0] == SWITCH_FAMILY) &&
          (devices_list[devices_tai8570w1+i].id[0] == SWITCH_FAMILY))) ;
}

int
devices_have_gpc(int i)
{
  if (devices_remote_used)
    return devices_remote_gpc[i] ;

  return (devices_list[devices_GPC1+i].id[0] == COUNT_FAMILY);
}

int
devices_have_any_gpc(void)
{
  int i ;

  for (i=0; i<MAXGPC; ++i)
    if (devices_have_gpc(i)) return 1 ;

  return 0 ;
}

/**
 * Do we have a solar sensor to read?
 */
int devices_have_solar_sensor(int i)
{
  if (devices_remote_used)
      return 0 ;

   return (devices_list[devices_sol1+i].id[0] == SBATTERY_FAMILY);
}

/**
 * Do we have a solar value?
 */
int devices_have_solar_data(int i)
{
  if (devices_remote_used)
    return devices_remote_solar[i] ;

  return (
	  ((i==0) && (devices_have_wsi603a())) ||
	  (devices_list[devices_sol1+i].id[0] == SBATTERY_FAMILY));
}

/**
 * Do we have a UV sensor to read?
 */
int devices_have_uv_sensor(int i)
{
  if (devices_remote_used)
      return 0 ;

   return (devices_list[devices_uv1+i].id[0] == HOBBYBOARDS_FAMILY);
}

/**
 * Do we have a UV value?
 */
int devices_have_uv_data(int i)
{
  if (devices_remote_used)
    return devices_remote_uv[i] ;

  return (devices_list[devices_uv1+i].id[0] == HOBBYBOARDS_FAMILY);
}

int
devices_have_adc(int i)
{
  if (devices_remote_used)
    return devices_remote_adc[i] ;

  return (devices_list[devices_adc1+i].id[0] == ADC_DS2760_FAMILY);
}

int
devices_have_thrmcpl(int i)
{
  if (devices_remote_used)
    return devices_remote_tc[i] ;

  return (devices_list[devices_tc1+i].id[0] == ADC_DS2760_FAMILY);
}

/**
 * Do we have a moisture board to read?
 */
int devices_have_moisture_board(int i)
{
  if (i >= MAXMOIST) return 0;
  if (devices_remote_used)
      return 0 ;

   return (devices_list[devices_moist1+i].id[0] == HOBBYBOARDS_NO_T_FAMILY);
}

/**
 * Do we have a soil moisture value?
 */
int devices_have_soil_moist(int i)
{
  int board = i/4;

  if (devices_remote_used)
      return 0 ;

  if (devices_have_moisture_board(board))
  {
	return (((moist_struct *)devices_list[devices_moist1+board].local)->type[i%4]==HBMOIST_SOIL);
  }

  return 0;
}

/**
 * Do we have a leaf wetness value?
 */
int devices_have_leaf_wet(int i)
{
  int board = i/4;

  if (devices_remote_used)
      return 0 ;

  if (devices_have_moisture_board(board))
  {
	return (((moist_struct *)devices_list[devices_moist1+board].local)->type[i%4]==HBMOIST_LEAF);
  }

  return 0;
}

int
devices_have_lcd(int i)
{
  if (devices_remote_used)
    return 0 ;

  return (devices_list[devices_lcd1+i].id[0] == PIO_FAMILY);
}

int devices_have_wsi603a(void)
{
  /* Do we have a WSI603A? */
  return (devices_list[devices_wsi603a].id[0] == ADC_DS2760_FAMILY) ;
}

/* Do we have a wind vane of any sort? */
/* Returns 1 if we do, otherwise 0 */
int
devices_have_vane(void)
{
  if (devices_remote_used)
    return devices_remote_vane ;

  /* Do we have a WSI603A? */
  if (devices_have_wsi603a()) return 1;

  /* Do we have a vane with a switch? */
  if (devices_list[devices_vane].id[0] == SWITCH_FAMILY) return 1 ;

  /* Do we have an adc-based vane? */
  if (devices_list[devices_vane_adc].id[0] == ATOD_FAMILY) return 1 ;

  /* Do we have an ds2438 adc-based vane? */
  if (devices_list[devices_vane_adc].id[0] == SBATTERY_FAMILY) return 1 ;

  /* Do we have a hobby boards ADS vane? */
  if (devices_list[devices_vane_ads].id[0] == SBATTERY_FAMILY) return 1 ;

  /* No vane found */
  return 0 ;
}

int devices_have_anem(void)
{
  if (devices_remote_used)
	return devices_remote_vane;

  /* Do we have a WSI603A? */
  if (devices_have_wsi603a()) return 1;

  /* Do we have an anem counter? */
  if (devices_list[devices_anem].id[0] == COUNT_FAMILY) return 1;

  return 0;
}

/* Do we have arain gauge? */
/* Returns 1 if we do, otherwise 0 */
int
devices_have_rain(void)
{
  if (devices_remote_used)
    return devices_remote_rain ;

  if (devices_list[devices_rain].id[0] == COUNT_FAMILY) return 1 ;

  /* No rain gauge found */
  return 0 ;
}

/* Do we have at least one thermometer or RH sensor? */
int
devices_have_something(void)
{
  int i ;

  if (devices_remote_used)
  {
    for (i=0; i<MAXHUMS; ++i)
      if (devices_remote_RH[i]) return 1 ;

    for (i=0; i<MAXBAROM; ++i)
      if (devices_remote_barom[i]) return 1 ;

    for (i=0; i<MAXGPC; ++i)
      if (devices_remote_gpc[i]) return 1 ;

    for (i=0; i<MAXTEMPS; ++i)
      if (devices_remote_T[i]) return 1 ;

    for (i=0; i<MAXSOILTEMPS; ++i)
      if (devices_remote_soilT[i]) return 1 ;

    for (i=0; i<MAXINDOORTEMPS; ++i)
      if (devices_remote_indoorT[i]) return 1 ;

    for (i=0; i<MAXSOL; ++i)
      if (devices_remote_solar[i]) return 1 ;

    for (i=0; i<MAXADC; ++i)
      if (devices_remote_adc[i]) return 1 ;

    for (i=0; i<MAXTC; ++i)
      if (devices_remote_tc[i]) return 1 ;

  }
  else
  {
    for (i=0; i<MAXHUMS; ++i)
      if (devices_have_hum(i)) return 1 ;

    for (i=0; i<MAXBAROM; ++i)
      if (devices_have_barom(i)) return 1 ;

    for (i=0; i<MAXGPC; ++i)
      if (devices_have_gpc(i)) return 1 ;

    for (i=0; i<MAXTEMPS; ++i)
      if (devices_have_temperature(i)) return 1 ;

    for (i=0; i<MAXSOILTEMPS; ++i)
      if (devices_have_soil_temperature(i)) return 1 ;

    for (i=0; i<MAXINDOORTEMPS; ++i)
      if (devices_have_indoor_temperature(i)) return 1 ;

    for (i=0; i<MAXSOL; ++i)
      if (devices_have_solar_sensor(i)) return 1 ;

    for (i=0; i<MAXADC; ++i)
      if (devices_have_adc(i)) return 1 ;

    for (i=0; i<MAXTC; ++i)
      if (devices_have_thrmcpl(i)) return 1 ;
  }

  return 0 ;
}

void
devices_clear_remote(void)
{
  int i ;

  for (i=0; i<MAXTEMPS; ++i)
    devices_remote_T[i] = 0 ;

  for (i=0; i<MAXSOILTEMPS; ++i)
    devices_remote_soilT[i] = 0 ;

  for (i=0; i<MAXINDOORTEMPS; ++i)
    devices_remote_indoorT[i] = 0 ;

  for (i=0; i<MAXHUMS; ++i)
    devices_remote_RH[i] = 0 ;

  for (i=0; i<MAXBAROM; ++i)
    devices_remote_barom[i] = 0 ;

  for (i=0; i<MAXGPC; ++i)
    devices_remote_gpc[i] = 0 ;

  for (i=0; i<MAXSOL; ++i)
    devices_remote_solar[i] = 0 ;

  for (i=0; i<MAXADC; ++i)
    devices_remote_adc[i] = 0 ;

  for (i=0; i<MAXTC; ++i)
    devices_remote_tc[i] = 0 ;

  devices_remote_vane = devices_remote_rain = 0 ;
}

/* Turn off all couplers */

int
devices_all_couplers_off(void)
{
  uchar extra[3];

  if (devices_active_branch != -1)
    {
      if
        (!SetSwitch1F(devices_portnum_default(),
	              devices_list[
                        DevicesBranchNum(devices_active_branch)].id,
                      SWT1F_DischargeLines,
                      0,
                      extra,
                      1 /* Discharge lines to reset parasitically powered devices */))
       werr(WERR_DEBUG0,
           "SetSwitch1F discharge lines failed on %s",
           devices_branch_name(devices_active_branch)) ;
      msDelay(200) ;
      if
        (!SetSwitch1F(devices_portnum_default(),
	              devices_list[
                        DevicesBranchNum(devices_active_branch)].id,
                      SWT1F_AllLinesOff,
                      0,
                      extra,
                      1 /* Do reset */))
       werr(WERR_DEBUG0,
           "SetSwitch1F all lines off failed on %s",
           devices_branch_name(devices_active_branch)) ;
      else
        devices_active_branch = -1 ;
    }


    return 1 ; /* OK */
}

enum branch_state {
  branch_error = -1,
  branch_to_trunk = 0,
  branch_to_branch,
  branch_unchanged
} ;

/* Set up communications on trunk or brank for a given device

*/


static int
devices_set_trunk_or_branch(int device)
{
  uchar extra[3];

  werr(DEVICES_DBLEV,
       "devices_set_trunk_or_branch(\"%s\") -> %s",
       devices_list[device].menu_entry,
       devices_branch_name(devices_list[device].branch)) ;

  /* Check for change of coupler. */
  if (devices_list[device].branch != devices_active_branch)
  {
    /* Changing to trunk or new branch */

    werr(DEVICES_DBLEV,
         "Change to %s",
         devices_branch_name(devices_list[device].branch)) ;

    /* Switch off branch? */
    /* Check for change of coupler. Aux / Main doesn't matter */
    /* Turn off old branch */
    if (DevicesBranchNum(devices_list[device].branch) !=
        DevicesBranchNum(devices_active_branch))
    {
      if ((devices_active_branch != -1) &&
          (!SetSwitch1F(devices_portnum(device), devices_list[
                          DevicesBranchNum(devices_active_branch)].id,
                        SWT1F_AllLinesOff,
                        0,
                        extra,
                        1 /* Do reset */)))
        werr(WERR_DEBUG0,
             "SetSwitch1F all lines off failed on %s",
             devices_list[
               DevicesBranchNum(devices_active_branch)].menu_entry) ;
    }


    /* Turn on new branch */

    devices_active_branch = devices_list[device].branch ;

    if (devices_active_branch != -1)
    {
      /* Change to new branch */
      if (!SetSwitch1F(
            devices_portnum(device),
	    devices_list[DevicesBranchNum(devices_active_branch)].id,
            DevicesMainOrAux(devices_active_branch) ?
            SWT1F_SmartOnMain :
            SWT1F_SmartOnAux,
            2,
            extra,
            1 /* Do reset */))
      {
        werr(DEVICES_DBLEV,
           "SetSwitch1F smart on failed on %s",
           devices_branch_name(devices_list[device].branch)) ;
        return branch_error ;
      }

      return branch_to_branch ;
    }

    return branch_to_trunk ;
  }
  else
  {
    return branch_unchanged ;
  }
}

static int
devices_match_rom(int device)
{
   uchar TranBuf[9];
   int i;

   // reset the 1-wire if on trunk
   // Don't need this for branch - reset alrady by smart on
   if ((devices_list[device].branch == -1) &&
        !owTouchReset(devices_portnum(device)))
     return FALSE ;

   // create a buffer to use with block function
   // match Serial Number command 0x55
   TranBuf[0] = 0x55;
   // Serial Number
   for (i = 1; i < 9; i++)
      TranBuf[i] = devices_list[device].id[i-1];

   // send/recieve the transfer buffer
   if (owBlock(devices_portnum(device), FALSE,TranBuf,9))
   {
      // verify that the echo of the writes was correct
      for (i = 1; i < 9; i++)
         if (TranBuf[i] != devices_list[device].id[i-1])
            return FALSE;
      if (TranBuf[0] != 0x55)
         return FALSE;
      else
         return TRUE;
   }


  // reset or match echo failed
  return FALSE;
}

static int
devices_skip_rom(int device)
{
   // reset the 1-wire if on trunk
   // Don't need this for branch - reset alrady by smart on
   if ((devices_list[device].branch == -1) &&
        !owTouchReset(devices_portnum(device)))
     return FALSE ;

   // Send Skip ROM command
   return owWriteByte(devices_portnum(device), 0xCC);
}

static int
devices_access_branch(int device)
{
  switch(devices_set_trunk_or_branch(device))
  {
    case branch_error:
      return FALSE ;

    case branch_to_trunk:
    case branch_unchanged:

      /* We need to generate a reset and a match ROM */
      /* Write SerialNum for this device */

      owSerialNum(devices_portnum(device), devices_list[device].id, FALSE) ;

      /* MLanAccess */

      if (owAccess(devices_portnum(device))) return TRUE ;

      /* MLanAccess failed */
      break ;

    case branch_to_branch:
      /* Smart on will have generated reset on branch already */
      /* Write SerialNum for this device */

      owSerialNum(devices_portnum(device), devices_list[device].id, FALSE) ;
      if (devices_match_rom(device)) return TRUE ;

      /* Failed to contact device */
      break ;
  }

  return FALSE ;
}

/** Switch on branch (or trunk) and issue Skip ROM command
*
* @param device - A device on the brank (or trunk) we want to access
* @return 1 on successful completion, 0 otherwise
*/
int
devices_skip_rom_branch(int device)
{
  if (devices_set_trunk_or_branch(device) == branch_error)
    return FALSE ;

  return devices_skip_rom(device) ;
}

/* Get ready to access device
   Activate coupler if necesary
   Set SerialNum

   Returns 1 if OK or 0 on failure
*/

int
devices_access(int device)
{
  /* Set up communications on trunk or brank */

  if (devices_access_branch(device))
  {
    return TRUE ;
  }

  werr(
    WERR_DEBUG0,
    "%s: access failed [1st go]",
    devices_list[device].menu_entry) ;

  /* MLanAccess failed
     Maybe the coupler dropped out
     Let's assume branch is not active */

  devices_active_branch = -1 ;
  if (devices_access_branch(device))
  {
    return TRUE ;
  }

  werr(
    WERR_DEBUG0,
    "%s: access failed [2nd go]",
    devices_list[device].menu_entry) ;

  /* If we're on a branch, discharge the lines and have another go */
  if ((devices_active_branch != -1) &&
       devices_all_couplers_off() &&
       devices_access_branch(device))
    return TRUE ;

  /* We're in big trouble */
  /* Shut down the port and start again */

  werr(WERR_DEBUG0,
    "%s: access still failing - Reinitializing serial port",
    devices_list[device].menu_entry) ;
  weather_shutdown() ;
  weather_acquire() ;

  /* Last attempt */
  return devices_access_branch(device) ;
}

/* Check if "family" is included in null-terminated list in device spec */

int
devices_match_family(devices_struct *device, unsigned char family)
{
  int i ;

  for (i=0; i<DEVICES_MAX_FAMILY; ++i)
  {
    /* Give up at list termination */
    if (!device->family_list[i]) return 0 ;

    if (device->family_list[i] == family) return 1 ; /* Found */
  }

  return 0 ; /* Not found */
}

/**
 * Do anything the devices system needs between reads, e.g. device reallocation
 *
 * return 0 for Ok, -1 for error
 */
int devices_poll()
{
  return devices_check_realloc();
}
