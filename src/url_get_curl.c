/*
*  C Implementation: url_get_curl
*
* Description: 
*
*
* Author: Simon Melhuish <simon@melhuish.info>, (C) 2005
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_LIBCURL

#include <curl/curl.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SELECT_H
/* since so many tests use select(), we can just as well include it here */
#include <sys/select.h>
#endif
#ifdef HAVE_UNISTD_H
/* at least somewhat oldish FreeBSD systems need this for select() */
#include <unistd.h>
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#include <time.h>
#include <assert.h>

#include "werr.h"
#include "url_get.h"
#include "intl.h"

#define BUFSIZE 2048
#define TIMEOUT 30

#ifdef HAVE_LIBCURL

static CURLM *multi_handle;
static int running = 0 ;
static int curl_init_done = 0 ;

static void url_update_status(void);
static size_t url_get_write(void *buffer, size_t size, size_t nmemb, void *userp);

static void
url_free_body(url_session *urls)
{
  if (urls->body)
  {
    free(urls->body) ;
    urls->body = NULL ;
  }
  urls->body_alloc = 0 ;
}

static void
url_curl_init(void)
{
  curl_global_init(CURL_GLOBAL_ALL);
  
  multi_handle = curl_multi_init();

  curl_init_done = 1 ;
}

void
url_init_session(url_session *urls)
{
  if (!curl_init_done)
    url_curl_init() ;
  
  /* Set sane starting values ready for next URL fetch */

  /*urls->url[0] = '\0' ;*/
  //urls->session = -1 ;

  if (urls->request == NULL)
  {
    urls->request = curl_easy_init();
    urls->status = -2  ;
    curl_easy_setopt(urls->request, CURLOPT_WRITEFUNCTION, url_get_write);
    curl_easy_setopt(urls->request, CURLOPT_WRITEDATA, urls);
    curl_easy_setopt(urls->request, CURLOPT_PRIVATE, (char *) urls);

    //printf("New request handle %p\n", urls->request);
  }
    
  url_free_body(urls) ;
  urls->count = 0 ;
  urls->idle = 0 ;
}

void
url_get_reinit(url_session *urls)
{
  url_init_session(urls) ;
  urls->status = -2  ;
}

void
url_deregister(url_session *urls)
{
  /* Done with this URL */

  if (url_get_any_session(urls))
  {
    curl_multi_remove_handle(multi_handle, (CURL *) urls->request) ;
    curl_easy_cleanup((CURL *) urls->request);
    urls->request = NULL ;
  }

  urls->status = -2 ;
}

int
url_failure(url_session *urls)
{
  url_deregister(urls) ;
  url_free_body(urls) ;
  return -1 ;
}

/* Get the status of each handle in the multi stack */
static void
url_update_status(void)
{
  CURLMsg *msg ;
  int msgs_in_queue ;
  
  do {
    msg = curl_multi_info_read(multi_handle, &msgs_in_queue);
    if (msg)
    {
      if (msg->msg == CURLMSG_DONE)
      {
        url_session *urls = NULL ;
        int res ;

        res = curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, (char **)&urls);

        if (res == CURLE_OK)
        {
          urls->status = msg->data.result;
          werr(WERR_DEBUG1, "status update - %s status %d", urls->url, urls->status);
  
          /* Remove this handle from the multi stack */
          curl_multi_remove_handle(multi_handle, msg->easy_handle);
  
          /* If we have any kind of error condition clear the body */
          if (urls->status != CURLE_OK)
          {
            url_free_body(urls) ;
          }
        }
        else
        {
          werr(WERR_DEBUG0, "curl_easy_getinfo failed - %s",curl_easy_strerror(res));
        }
      }
    }
  } while (msg);
}

int
url_request(url_session *urls, char *url, char *proxy)
{
  CURLMcode res;

  //printf("url_request(%p, \"%s\", \"%s\"\n", urls, url, proxy);
  
  if (!url) return -1 ;
  if (!url[0]) return -1 ;
    
  /* Is a fetch running already? */
  if (urls->status != -2)
  {
    werr(WERR_DEBUG0, "old http session still running - restart: %s", urls->url);
    url_stop(urls) ;
    url_failure(urls) ;
//    return 0 ;
  }

  url_init_session(urls) ;

  time(&urls->idle) ; /* Start idle timer */

  urls->status = -1 ; // Running this request

  if (proxy)
  {
    /* Enable the proxy */

    curl_easy_setopt(urls->request, CURLOPT_PROXY, proxy);
  }

  /* Set the URL */

  werr(WERR_DEBUG0, "Fetch URL \"%s\"", url) ;

  curl_easy_setopt(urls->request, CURLOPT_URL, url);

  /* Add this session's handle to the multi stack */
  res = curl_multi_add_handle(multi_handle, urls->request);
  ++running ;

  //printf("curl_multi_add_handle -> %d\n", res);

  if (res == CURLM_CALL_MULTI_PERFORM)
    /* Run the mult fetch right away, to update the running count at least */
    res = curl_multi_perform(multi_handle, &running);
    werr(WERR_DEBUG1, "curl_multi_perform -> %d, running = %d", res, running);
    url_update_status();

  // CURLM_CALL_MULTI_PERFORM=-1
  // CURLM_OK=0
  // >CURLM_OK => error

  // We return -1 on error
  
  return (res > CURLM_OK) ? -1 : 0 ;
}

void
url_stop(url_session *urls)
{
  /* Halt attempt to fetch URL - e.g. because it timed out */

  //ghttp_close((ghttp_request *) urls->request) ;
  /* Remove this session's handle from the multi stack */
  curl_multi_remove_handle(multi_handle, urls->request);
}

int url_finished(url_session *urls)
{
  /* Has the fetch finished? */

  //return (urls->status != ghttp_not_done) ;
  //printf("url_finished -> %d\n", (urls->status >= 0));
  return (urls->status >= 0) ;
}

/* Fetch any available data for handles in the multi stack.
Their callback functions will be called, updating data in memory. */

char *url_get_body_part(url_session *urls)
{
  /* We don't use urls - all the handles in the multi stack will be tried on each call */
  if (running > 0)
  {
    int old_running = running ;
    curl_multi_perform(multi_handle, &running);
    url_update_status();
    /*if (running != old_running)
      printf("http running %d\n", running);*/
  }

  return NULL ; // We don't actually use the return value
}


/* This is the callback function for curl */
static size_t
url_get_write(void *buffer, size_t size, size_t nmemb, void *userp)
{
  url_session *urls ;
  int len ;

  werr(WERR_DEBUG1, "url_get_write RX");

  urls = (url_session *) userp ;

  if (urls->body)
    url_free_body(urls) ;

  len = size * nmemb ;

  urls->body = (char *) realloc(urls->body, urls->body_alloc+len+1) ;

  strncpy(&(urls->body[urls->body_alloc]), buffer, len) ;
  urls->body_alloc += len ;

  urls->body[urls->body_alloc] = '\0' ;

  return len ;

  /* Done */
}



  
/* Is there an active session? */

int
url_get_any_session(url_session *urls)
{
  if (!urls) return 0 ;
  if (!urls->request) return 0 ;
  
  return (urls->status == -1) ;
}

#else
/* We do not have libcurl */

void
url_stop(url_session *urls)
{
  return ;
}

void
url_deregister(url_session *urls)
{
  return ;
}

void
url_init_session(url_session *urls)
{
  return ;
}

int
url_request(url_session *urls, char *url, char *proxy)
{
  werr(0, _("Oww was not compiled with libghttp.")) ;
  return -1 ;
}

int
url_failure(url_session *urls)
{
  return -1 ;
}

char *
url_get_body_part(url_session *urls)
{
  return NULL ;
}

int
url_finished(url_session *urls)
{
  return 1 ;
}

int
url_get_any_session(url_session *urls)
{
  return 0 ;
}

void
url_get_reinit(url_session *urls)
{
  return ;
}

#endif
