/* sendwx.h */

#ifndef SENDWX_H
#define SENDWX_H 1

#define STN_NAME_MAX 40

typedef struct {
  char id[17] ; /* Station id */
  char name[STN_NAME_MAX] ; /* Station name */
} sendwx_stn_info ;

enum wund_types {
	WUND_OFF = 0,
	WUND_NORMAL,
	WUND_RAPID
};

enum wow_types {
	WOW_OFF = 0,
	WOW_NORMAL,
	WOW_RAPID
};

extern sendwx_stn_info *sendwx_stn_list ;
extern int              sendwx_stn_list_count ;
extern char             sendwx_dallasremote_name[] ;

time_t convert_time(time_t t) ;
int sendwx_poll(void) ;
void sendwx_send(void) ;
int sendwx_parse_dallas_list(char *body, void *test_data) ;
int sendwx_parse_dallas_rx(char *body, void *test_data) ;
void sendwx_recv_list(int (*test_func)(char *, void*)) ;
void sendwx_kill_dallas_rx(void) ;
void sendwx_recv_data(int (*test_func)(char *, void*)) ;


#endif
