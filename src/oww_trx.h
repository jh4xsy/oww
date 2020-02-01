/* oww_trx.h

   Data transfer using oww protocol

*/

/* Build an oww_trx message - don't send it */

int
oww_trx_tx_build(wsstruct *wd, int msg_type) ;

/* Build an oww_trx message and send to clients */

int
oww_trx_tx(wsstruct *, int msg_type) ;

/* Build an announcment message */

void *
oww_trx_build_announce(wsstruct *wd, int *len) ;

/* Receive WS data by TCP, using Oww protocol */
/* socket = tcp socket
   wsstruct = weather structure
   rxed_data = did we get data for an update?
*/

int
oww_trx_rx(int socket, wsstruct *wsp, int *msg_type) ;

#define OWW_TRX_VERSION "052" /* Version 0.52 */
#define OWW_TRX_BUFINC 64

#define OWW_TRX_END     0L /* To mark end of data */
#define OWW_TRX_TIME    0x1000000L /* Times */
#define OWW_TRX_WIND    0x2000000L /* Wind direction, speed, gust and max */
#define OWW_TRX_RAIN    0x3000000L /* Rain mono, rain, since, rate, in hour */
#define OWW_TRX_T       0x4000000L /* T (C) * 100 or Trh (C) * 100 */
#define OWW_TRX_RH      0x6000000L /* RH (%) * 100 */
#define OWW_TRX_BP      0x7000000L /* BP (mB) * 10 */
#define OWW_TRX_SOLAR   0xb000000L /* Solar - arbitrary units (float) */
#define OWW_TRX_UVI     0xc000000L /* UVI -
integral of UV flux density weighted by McKinlay-Diffey Erythema action spectrum
divided by 10 mW/(nm m^2) (float) */
#define OWW_TRX_ADC     0xd000000L /* ADC values */
#define OWW_TRX_MOIST   0xe000000L /* Soil moisture - mBar */
#define OWW_TRX_LEAF    0xf000000L /* Leaf wetness - arbitrary units */

#define OWW_TRX_UPDT    0x80000000L /* Update interval only */

#define OWW_TRX_GPC     0x90000000L /* gpc, gpcdelta, gpcmono, gpcrate,
                                       gpcch, gpcmaxch */

#define OWW_TRX_LOC     0xa0000000L /* location */

#define OWW_TRX_HEAD    0xff000000L /* To mark header of normal data */

#define OWW_TRX_SUB_TRH     0x10000L /* Trh subtype of T */
#define OWW_TRX_SUB_TB      0x20000L /* Tb subtype of T */
#define OWW_TRX_SUB_TUV     0x30000L /* Tuv subtype of T */
#define OWW_TRX_SUB_TSOL    0x40000L /* Tsol subtype of T */
#define OWW_TRX_SUB_TADC    0x50000L /* Tadc subtype of T */
#define OWW_TRX_SUB_TTC     0x60000L /* Ttc subtype of T */
#define OWW_TRX_SUB_TCJ     0x70000L /* Tcj subtype of T */
#define OWW_TRX_SUB_TSOIL   0x80000L /* Tsoil subtype of T */
#define OWW_TRX_SUB_TINDOOR 0X90000L /* Tindoor subtype of T */

#define OWW_TRX_SERIAL  0x100L /* For serial number byte */

#define OWW_TRX_SIZE    1L /* Size byte */


/* Message types */

#define OWW_TRX_MSG_WSDATA 1
#define OWW_TRX_MSG_UPDT   2
#define OWW_TRX_MSG_MORT   3
#define OWW_TRX_MSG_ANNOUN 4

