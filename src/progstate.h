enum prog_states {
  state_startup = 0,
  state_ready,
  state_waiting,
  state_readanemstart,
  state_starttempconv,
  state_waiting_tempconv,
  state_endtempconv,
  state_read_humidity,
  state_read_barometer,
  state_read_gpc,
  state_read_solar,
  state_read_uv,
  state_read_adc,
  state_read_tc,
  state_read_moist,
  state_readanemend,
  state_read_ws_done,
  state_idle,
  state_port_ok,
  state_wsdead,
  state_learn,
  state_newcom,
  state_zombie,
  state_devclaim,
  state_noport,
  state_remote_connect,
  state_remote_read,
  state_remote_done,
  state_datasource_none,
  state_dallas_startrx,
  state_dallas_rx,
  state_remote_zombie,
  state_restart,
  STATE_STATES
} ;

extern int prog_state, prog_state_old ;
extern const char *prog_states[] ;

const char *state_get_name(int i) ;
void state_new(int new_state) ;
int state_machine(void) ;
void state_change_datasource(int new_source) ;
void state_machine_exit_request(int urgent);
void state_machine_quit();

#define SMTEST_AT_NOW   0L
#define SMTEST_AT_ASAP  1L
#define SMTEST_AT_LATER 2L

#define DWELL_TIME 2L
