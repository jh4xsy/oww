void applctn_pre_init(int *argc, char ***argv) ;
void applctn_init(int *argc, char ***argv) ;
int applctn_startup_finished(void) ; /* Function to call in state_startup */
void applctn_polling_loop(void) ;
void applctn_smtest_at(int64_t next_test) ;
void applctn_quick_poll(unsigned long int when) ;

void applctn_go_daemon(void);

extern int applctn_gui, applctn_quit ;
