/* mainwin.h */

/* Functions to manipulate the main display window */

void mainwin_update(int new_data) ;
void mainwin_init(void *id) ;
void mainwin_check_animation(unsigned long int time_now) ;
extern int mainwin_kill_anim ;

#define TSTEM_HEIGHT 24
#define TSTEM_WIDTH   8
#define TBULB_X0    100
#define TBULB_Y0    176
#define TBULB_RAD    10
#define SUN_X0     60
#define SUN_Y0     60
#define SUN_RAD    15
#define UVI_X0 45
#define UVI_Y0 80
#define UVI_WIDTH  30
#define UVI_HEIGHT 10
