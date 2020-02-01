typedef struct {
  char url[512] ;
  int (*test)(char *, void*) ;
  void *test_data ;
  int session ;
  void *request ;
  int status  ;
  char *body ;
  int count ;
  int body_alloc ;
  time_t idle ;
} url_session ;

void url_stop(url_session *urls) ;
/*void url_deregister(url_session *urls) ;*/
int url_failure(url_session *urls) ;
void url_init_session(url_session *urls) ;
void url_get_reinit(url_session *urls) ;
int url_request(url_session *urls, char *url, char *proxy) ;
char *url_get_body_part(url_session *urls) ;
int url_finished(url_session *urls) ;
int url_get_any_session(url_session *urls) ;
