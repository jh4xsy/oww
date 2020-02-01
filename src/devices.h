/* devices.h */

/*
   Prototypes for 1-wire devices

   Oww project
   Dr. Simon J. Melhuish

   Tue 12th December 2000
*/

#ifndef DEVICES_H
#define DEVICES_H 1
#include "setupp.h"
#include "nstats.h"

enum device_types {
  devtype_unspec = 0,
  devtype_T,
  devtype_vane,
  devtype_anem,
  devtype_rain,
  devtype_RH,
  devtype_barom,
  devtype_gpc,
  devtype_solar,
  devtype_uv,
  devtype_adc,
  devtype_tc,
  devtype_tai8570w,
  devtype_lcd,
  devtype_branch,
  devtype_wsi603a,
  devtype_moisture,
  devtype_vane_cal

  //devtype_tai8570r
} ;

#define DEVCALIB 5
#define DEVICES_MAX_FAMILY 10
#define DEVICES_MAX_SEARCH 256

/* struct for info on a weather station element */
typedef struct _devstruct {
  unsigned char id[12] ; /* ID bytes for this device */
  //unsigned char family ; /* Family of 1-wire devices that may be used */
  unsigned char family_list[DEVICES_MAX_FAMILY] ; /* List of allowed family codes */
  int branch ;           /* Branch number -1 => root */
  int back_alloc ;       /* Which discover list member was allocated here? */
  int required ;         /* Is this device vital for basic operation? */
  char menu_entry[32] ;  /* Choice for user */
  int devtype ;          /* What kind of device is this? */
  float calib[DEVCALIB] ;  /* Calibration */
  int ncalib ;           /* No. of calib values in use */
  void *(*stats_maker)(struct _devstruct *); /* Function to make stats struct */
  void *local ;          /* Local data for device */
  int (*parser_special)(int); /* Function to be called by parser */
} devices_struct ;

typedef struct {
  int devtype ;          /* What kind of device is this?
                            Should match type in devices_struct */
  int status ;
  int (*update)(devices_struct *dev) ;
} devices_local_struct ;

enum devices_status {
  devices_status_ignore = 0,
  devices_status_update,
  devices_status_ready
} ;


#define DefCalib {1.0F, 0.0F, 0.0F, 0.0F, 0.0F}

/* Array to hold IDs, &c, for use by weather.c (&c) functions */
extern devices_struct *devices_list ;

extern int devices_vane ;
extern int devices_vane_adc ;
extern int devices_anem ;
extern int devices_wsi603a ;
extern int devices_vane_ads ;
extern int devices_vane_ins ;
extern int devices_rain ;
extern int devices_T1 ;
extern int devices_soilT1 ;
extern int devices_indoorT1 ;
extern int devices_H1 ;
extern int devices_BAR1 ;
extern int devices_tai8570w1 ;
extern int devices_GPC1 ;
extern int devices_sol1 ;
extern int devices_uv1 ;
extern int devices_adc1 ;
extern int devices_tc1 ;
extern int devices_moist1 ;
extern int devices_lcd1 ;
extern int devices_brA ;
extern int devices_wv0 ;
extern int DEVICES_TOTAL ;

/* The order of elements in devices_list */
/*enum devices_names {
  devices_vane = 0,
  devices_vane_adc,
  devices_vane_ads,
  devices_anem,
  devices_rain,
  devices_T1,
  devices_T2,
  devices_T3,
  devices_T4,
  devices_T5,
  devices_T6,
  devices_T7,
  devices_T8,
  devices_H1,
  devices_H2,
  devices_H3,
  devices_H4,
  devices_H5,
  devices_H6,
  devices_H7,
  devices_H8,
  devices_BAR1,
  devices_BAR2,
  devices_BAR3,
  devices_BAR4,
  devices_BAR5,
  devices_BAR6,
  devices_BAR7,
  devices_BAR8,
  devices_tai8570w1,
  devices_tai8570w2,
  devices_tai8570w3,
  devices_tai8570w4,
  devices_tai8570w5,
  devices_tai8570w6,
  devices_tai8570w7,
  devices_tai8570w8,
  devices_GPC1,
  devices_GPC2,
  devices_GPC3,
  devices_GPC4,
  devices_GPC5,
  devices_GPC6,
  devices_GPC7,
  devices_GPC8,
  devices_sol1,
  devices_sol2,
  devices_sol3,
  devices_sol4,
  devices_adc1,
  devices_adc2,
  devices_adc3,
  devices_adc4,
  devices_brA,
  devices_brB,
  devices_brC,
  devices_brD,
  devices_brE,
  devices_brF,
  devices_brG,
  devices_brH,
  devices_brI,
  devices_wv0,
  devices_wv1,
  devices_wv2,
  devices_wv3,
  devices_wv4,
  devices_wv5,
  devices_wv6,
  devices_wv7,
  DEVICES_TOTAL
} ;*/

extern int DEVICES_TOTAL ;

extern int
devices_remote_T[],
devices_remote_soilT[],
devices_remote_indoorT[],
devices_remote_RH[],
devices_remote_barom[],
devices_remote_gpc[],
devices_remote_solar[],
devices_remote_uv[],
devices_remote_adc[],
devices_remote_tc[],
devices_remote_soilmoisture[],
devices_remote_leafwetness[],
devices_remote_vane,
devices_remote_rain,
devices_remote_used ;


/* Use bit 6 to flag main/aux branch */
#define DevicesMainOrAux(x) (x/128)
#define DevicesBranchNum(x) (x%128)
#define DevicesBranchCode(x,y) (x+y*128)

/* struct to hold the results of searches */
typedef struct {
  unsigned char id[12] ;   /* ID bytes for this device */
  int branch ;             /* Branch number -1 => root */
  int alloc ;              /* To which WS element is this allocated? */
} devices_search_struct ;

/* How many devices are in the search list? */
extern int devices_known ;

/* The list of search results and their allocations */
extern devices_search_struct *devices_search_list ;

// /* What is the default slope for this device? */
// float devices_default_slope(devices_struct *device) ;
//
// /* What is the default offset for this device? */
// float devices_default_offset(devices_struct *device) ;

/** Build the array to had devices
*
*/
int devices_build_devices_list(void) ;

// /** Take the id for a device and copy to a string
// *
// */
// int devices_id_to_string(char *string, setupp_liststr *member, int index) ;
//
// /** Read a device string and convert it to an id
// *
// */
// int devices_string_to_id(char *string, setupp_liststr *member, int index) ;

/** Check vane id allocations
*
*/
void devices_check_vane_ids(void) ;

/** Wipe the list of vane IDs
*/
void devices_wipe_vanes(void);

/* Get the port number for a specific device */
int devices_portnum(int device) ;

/* Get the general port number */
int devices_portnum_default(void) ;

// Global for portnum hack
extern int devices_portnum_var ;

/* Text for cal menu */
char *
devices_cal_name(int devtype, int i) ;

/* Copy one ID array to another */
int
devices_copy_ids(unsigned char to_id[], unsigned char from_id[]) ;

/* Compare the IDs of two devices 1 => equal */
int
devices_compare_ids(unsigned char id1[], unsigned char id2[]) ;

/* Allocate 'from' in search list to 'to' in device list */
int
devices_allocate(int to, int from) ;

/* Scan the device list to make a suitable allocation */
int
devices_auto_allocate(int num) ;

/* Clear out the search list - free memory, &c */
int
devices_clear_search_list(void) ;

/* Clear IDs of device list entries with no back alloc */
int
devices_purge_devices_list(void) ;

/* devices_purge_search_list

   Remove any missing devices from the search list */
/*int
devices_purge_search_list(void) ;*/

/* Remove an entry (num) from the search list */
/* wipe_id == 1 => blank out corresponding ID in device list */
int
devices_remove_search_entry(int num, int wipe_id) ;

/* Expand search list and add an ID */
int
devices_new_search_entry(unsigned char id[], int branch, int do_allocation) ;

/* Search for 1-wire devices from family and add to search list */
int
devices_search_and_add(unsigned char family, int do_allocation) ;

/* In normal use we get devices_list from devices file */
/* Build a fake search list from this information */
/* add_all => cover whole list, not just wv* */
int
devices_build_search_list_from_devices(int add_all) ;

///* Allocate a search list entry by name, swapping if necessary */
///* search_entry - index into devices_search_list */
///* target       - name of target in devices_list */
///* wipe         - do we clear target ID for --None-- ? */
//int
//devices_reallocate(int search_entry, char *target, int wipe) ;

/* Return a string corresponding to the family type */
char *
devices_get_type(unsigned char family) ;

/* Do we have thermometer i? */
int
devices_have_thermometer(int i) ;

/* Do we have temperature i? Could come from another instrument */
int
devices_have_temperature(int i) ;

/* Do we have temperature i? Could come from another instrument */
int
devices_have_soil_temperature(int i) ;

/* Do we have temperature i? Could come from another instrument */
int
devices_have_indoor_temperature(int i) ;

/* Do we have humidity sensor i? */
int
devices_have_hum(int i) ;

/* Do we have barometer sensor i? */
int
devices_have_barom(int i) ;

/* Do we have general purpose counter i? */
int
devices_have_gpc(int i) ;

/* Do we have any gpcs? */
int
devices_have_any_gpc(void) ;

/* Do we have a solar sensor to read? */
int devices_have_solar_sensor(int i);

/* Do we have solar sensor i? */
int
devices_have_solar_data(int i) ;

/* Do we have a UV sensor to read? */
int devices_have_uv_sensor(int i);

/* Do we have adc sensor i? */
int
devices_have_adc(int i);

/* Do we have thermocoupler i? */
int
devices_have_thrmcpl(int i);

/**
 * Do we have a moisture board to read?
 */
int devices_have_moisture_board(int i);

/**
 * Do we have a soil moisture value?
 */
int devices_have_soil_moist(int i);

/**
 * Do we have a leaf wetness value?
 */
int devices_have_leaf_wet(int i);

/* Do we have lcd i? */
int
devices_have_lcd(int i) ;

/* Do we have a WSI603A combined weather instrument? */
int devices_have_wsi603a(void);

/* Do we have a wind vane of any sort? */
/* Returns 1 if we do, otherwise 0 */
int
devices_have_vane(void) ;

/* Do we have an anemometer of any sort? */
/* Returns 1 if we do, otherwise 0 */
int devices_have_anem(void);

/* Do we have arain gauge? */
/* Returns 1 if we do, otherwise 0 */
int
devices_have_rain(void) ;

/* Do we have at least one thermometer or RH sensor? */
int
devices_have_something(void) ;

/* Clear flags for remote devices */
void
devices_clear_remote(void) ;

/* Turn off all couplers */

int
devices_all_couplers_off(void) ;

/* Set up communications on trunk or brank for a given device

   Returns 0 on failure, 1 otherwise
*/

/*int
devices_set_trunk_or_branch(int device) ;*/

/* Get ready to access device
   Activate coupler if necesary
   Set SerialNum

   Returns 1 if OK or 0 on failure
*/

/** Switch on branch (or trunk) and issue Skip ROM command
*
* @param device - A device on the brank (or trunk) we want to access
* @return 1 on successful completion, 0 otherwise
*/
int devices_skip_rom_branch(int device) ;

int
devices_access(int device) ;

/* Check if "family" is included in null-terminated list in device spec */

int
devices_match_family(devices_struct *device, unsigned char family) ;

/**
 * Do anything the devices system needs between reads, e.g. device reallocation
 *
 * return 0 for Ok, -1 for error
 */
int devices_poll();

int devices_queue_realloc(int search_entry, char *target, int wipe, void (*callback)(void));

#endif
