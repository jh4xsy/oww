/* Update the devices list */
void setupd_build_scrolllist(void);

/* Update devices window if more wind vane IDs found */
int setupd_poll(void) ;

/* Load setup and set inital setup window entries */
void setupd_window_created(int /* window ID*/) ;

/* Load devices and set inital devices window entries */
int setupd_devices_window_created(int /* window ID*/) ;

/* These functions for RISC OS only... */

#ifdef RISCOS
/* weather station was shut down - get new comm params and restart */
/*int setupd_new_com(void) ;*/

#endif
