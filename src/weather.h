/* weather.h */
#include "stats.h"

void weather_initialize_vals(void) ;
int weather_acquire(void) ;
int weather_init(void) ;
int weather_shutdown(void) ;
float weather_primary_T(statsmean *means) ;
int weather_primary_soilT(statsmean *means) ;
int weather_primary_indoorT(statsmean *means) ;
int weather_primary_rh(statsmean *means) ;
int weather_primary_barom(statsmean *means) ;
int weather_primary_solar (statsmean * means) ;
int weather_primary_soilmoisture(statsmean * means);
int weather_primary_adc (statsmean * means);
char *weather_rain_since(void) ;
void weather_reset_stats(void) ;
int weather_read_ws(void) ;
int weather_read_wsgust(void) ;
int FindWeatherStation(void) ;
void weather_lcd_output(void) ;
//int weather_read_ws_read_gpc (void) ;
void weather_sub_update_gpcs (void) ;

#define TEMP_CONV_TIME 100L
#define UNDEFINED_T -1000.0F
#define BAD_T 85.0F

/*#define TEMP_FAMILY        0x10
#define SWITCH_FAMILY      0x12
#define COUNT_FAMILY       0x1D
#define DIR_FAMILY         0x01*/
