/* arne.h

   Broadcasting weather station data by UDP
   according to protocol of Arne Henriksens

   http://weather.henriksens.net/
*/

extern int arne_wind_ready ;

int
arne_init_wind(void) ;

/* Broadcast WS data by UDP, accorning to Arne's protocol */

int
arne_tx(wsstruct *) ;

int
arne_new_wind_dir(int dir, time_t t) ;

int
arne_read_out_wind_dir(void) ;

int
arne_new_wind_speed(wsstruct *wd) ;

//int
//arne_rx_decode(char *buffer,  wsstruct *wd) ;

int
arne_rx(int socket, wsstruct *wsp, int *msg_type) ;

#define ARNE_RX_WSDATA 0
#define ARNE_RX_ANNOUNCE 1
