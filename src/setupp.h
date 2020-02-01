/* setupp.h */

/* Setup file reading / writing */

#ifndef SETUPP_H
#define SETUPP_H 1

#include "omem.h"

#define SETUPP_MAXTAG 20

enum setupp_types {
  SETUPP_SPECIAL = 0,
  SETUPP_INT,
  SETUPP_UINT,
  SETUPP_FLOAT,
  SETUPP_STRING,
  SETUPP_ULONG,
  SETUPP_ASTRING,
  SETUPP_ULONGARR
} ;

enum setupp_suffix_types {
  SETUPP_SUFFIX_NONE = 0,
  SETUPP_SUFFIX_0,
  SETUPP_SUFFIX_1,
  SETUPP_SUFFIX_A
} ;

#define SETUPP_COMMENTS "#|\n\r "

typedef struct setupp_liststr_{
  char tag[SETUPP_MAXTAG] ;     /* Value name */
  int suffix_type ;             /* Can the tag have a suffix? */
  int index_max ;              /* Maximum value for index */
  int  type ;                   /* Value type */
  void *data ;                  /* Pointer to data location */
  int data_len ;                /* Bytes allocated for data strings */
  int (*parsew)(char *, struct setupp_liststr_ *, int) ;      /* Special parser function - write */
  int (*parser)(char *, struct setupp_liststr_ *, int) ;      /* Special parser function - read */
  //stetupp_parser *parser ;      /* Special parser function - read  */
} setupp_liststr ;

typedef int (stetupp_parser) (char *string, setupp_liststr *member, int index) ;

int setupp_write(const char *name, setupp_liststr *list) ;

int setupp_read(const char *name, setupp_liststr *list) ;

/*int setupp_write_buffer(char *buffer, int buffsize,
  setupp_liststr *member) ;

int setupp_read_buffer(char *buffer, setupp_liststr *member) ;*/

int setupp_sreadline(char *line, setupp_liststr *member) ;
int setupp_swriteline(omem *mem, setupp_liststr *member, int index) ;

#endif

