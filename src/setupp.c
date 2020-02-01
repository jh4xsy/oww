/* setupp.c */

/*
 * For !OWW project
 * One-wire weather
 * Dr. Simon J. Melhuish
 * August - December 1999
 * Free for non-comercial use
 * Dallas parts subject to their copyright and conditions
 *
 */

/* Setup file reading / writing */

/* The format of the setup file is:

  tag value
  tag value
     ...
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#ifdef WIN32
#include "config-w32gcc.h"
#else
#include "../config.h"
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#ifndef isblank
#define isblank(x) ((x) == ' ' || (x) == '\t')
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#include "setupp.h"
#include "werr.h"
#include "globaldef.h"
#include "oww.h"
#include "omem.h"
#include "intl.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "utility.h"

static char *setupp_tag_with_suffix(setupp_liststr *member, char *tps, int index)
{
  //static char tps[SETUPP_MAXTAG+4] ;

  switch (member->suffix_type)
  {
    case SETUPP_SUFFIX_NONE:
      strcpy(tps, member->tag) ;
      return tps ;

    case SETUPP_SUFFIX_0: // Numeric suffix - 0-based
    {
      sprintf(tps, "%s%d", member->tag, index) ;
      return tps ;
    }

    case SETUPP_SUFFIX_1: // Numeric suffix - 1-based
    {
      sprintf(tps, "%s%d", member->tag, index+1) ;
      return tps ;
    }

    case SETUPP_SUFFIX_A: // Alphabetic suffix
    {
      sprintf(tps, "%s%c", member->tag, 'A'+(char) index) ;
      return tps ;
    }
  }
  return NULL ;
}

static int
setupp_match_tag(char *tag, setupp_liststr *member, int *index)
{
  char tps[SETUPP_MAXTAG+4] ;

  switch (member->suffix_type)
  {
    case SETUPP_SUFFIX_NONE:
      if (strcmp(tag, member->tag) == 0)
      {
        *index = 0 ;
        return 1 ;
      }
      return 0 ;

    case SETUPP_SUFFIX_0: // Numeric suffix - 0-based
    case SETUPP_SUFFIX_1: // Numeric suffix - 1-based
    case SETUPP_SUFFIX_A: // Alphabetic suffix
    {
      if (member->index_max < 1) return 0 ;
      for (*index=0; *index<member->index_max; ++*index)
      {
        if (strcmp(tag, setupp_tag_with_suffix(member, tps, *index)) == 0)
          return 1 ;
      }
      return 0 ;
    }
  }
  return 0 ;
}

int setupp_sreadline(char *line, setupp_liststr *member)
{
  /* Read a entry from the setup file */
  /* Return 1 for success, 0 for failure */

  char *data_string ;
  int index ;

  /* Get tag and data */
  //werr(WERR_WARNING, line) ;

  if (line==NULL) return 0; // Failed

  // Ignore leading spaces
  while (isblank(*line)) ++line;

  data_string = strpbrk(line, " \t") ;
  if (!data_string) return 1 ;

  *data_string++ = '\0' ; /* Terminate tag and advance to data */

  // Ignore leading spaces
  while (isblank(*data_string)) ++data_string;

  while (member->data) {
    /* Check for matching tag - ignore comment tags */
    if (!strchr(SETUPP_COMMENTS, line[0]) &&
        (setupp_match_tag(line, member, &index))) {
        //(strcmp(line, member->tag) == 0)) {
      /*char buff[64] ;*/
      /* Found a match - get data */

      /* What type is it? */

      switch (member->type) {
        case SETUPP_SPECIAL:
        {
          /*sscanf(data_string, "%s\n", temp_string) ;*/
          int i ;
          char *data_copy ;

          data_copy = strdup(data_string) ;

          //strcpy(temp_string, data_string) ;
          for (i=0; data_copy[i] >= ' '; ++i) ;
          data_copy[i] = '\0' ;
          member->parser(data_copy, member, index) ;
          free(data_copy) ;
          break ;
        }

        case SETUPP_INT:
          sscanf(data_string, "%d\n", (int *) member->data) ;
          break ;

        case SETUPP_UINT:
          sscanf(data_string, "%u\n", (unsigned int *) member->data) ;
          break ;

        case SETUPP_FLOAT:
          sscanf(data_string, "%f\n", (float *) member->data) ;
          break ;

        case SETUPP_STRING:
        {
          int ic = 0 ;

          while ((data_string[ic] != '\n') &&
            (data_string[ic] != '\0') && (ic < member->data_len-1)) {
            ((char *) member->data)[ic] = data_string[ic] ;
            ++ic ;
          }
          ((char *) member->data)[ic] = '\0' ;

          /*sscanf(data_string, "%s\n", (char *) member->data) ;*/
          /*printf("%s: %s\n", member->tag, (char *) member->data) ;*/
          break ;
        }

        case SETUPP_ULONG:
          /*sscanf(data_string, "%s\n", buff) ;*/
          *((unsigned long *) member->data) =
             strtoul(data_string, NULL, 10) ;
          /*sscanf(data_string, "%d\n", (int *) member->data) ;*/
          break ;

        case SETUPP_ULONGARR:
        {
          /* Scan for ulong entries to place into an array */
          char *next ;
          int i ;

          next = data_string ;

          for (i=0; ((i<member->data_len) && next); ++i)
          {
            ((unsigned long *) member->data)[i] =
               strtoul(next, &next, 10) ;
          }
          break ;
        }

        case SETUPP_ASTRING:
        {
          int ic = 0, i, j=0 ;
          char *out = NULL ;
          while ((data_string[ic] != '\n') && (data_string[ic] != '\0')) 
            ++ic ;
          out = (char *) malloc(sizeof(char) * (ic + 1)) ;
          if (!out) break ;
          for (i=0; i<ic; ++i) 
          {
            if (data_string[i] == '\\')
            {
              // Escaped character
              if (isdigit(data_string[++i]))
              {
                // Octal
                char buf[4];
                long int res;
                    
                buf[0] = data_string[i];
                buf[1] = data_string[++i];
                buf[2] = data_string[++i];
                buf[3] = '\0';
                    
                res = strtol(buf, NULL, 8);
                out[j] = (char) (0xff & res);
                break;
              }
              else
              {
                switch (data_string[i])
                {
                  case 'x': // Hex - two digits to read
                  {
                    char buf[3];
                    long int res;
                    
                    buf[0] = data_string[++i];
                    buf[1] = data_string[++i];
                    buf[2] = '\0';
                    
                    res = strtol(buf, NULL, 16);
                    out[j] = (char) (0xff & res);
                    break;
                  }
                    
                  case 'n': // Newline
                    out[j] = '\n';
                    break;
                    
                  case 't': // Tab
                    out[j] = '\t';
                    break;
                  
                  case 'v': // Vertical tab
                    out[j] = '\v';
                    break;
                    
                  case 'b': // Backspace
                    out[j] = '\b';
                    break;
                    
                  case 'r': // Carriage Return
                    out[j] = '\r';
                    break;
                    
                  case 'f': // Form feed
                    out[j] = '\f';
                    break;
                    
                  case '\\': // Blaen-slaes
                    out[j] = '\\';
                    break;
                    
                  default:
                    out[j] = data_string[i] ;
                }
              }
            }
            
            else
              out[j] = data_string[i] ;
            
            ++j;
          }
          out[j] = '\0' ;
          *((char **) member->data) = out ;
          break ;
        }
      }

      return 1 ;
    }

    ++member ;
  }

  werr(0, _("Error reading setup / devices file - unknown tag: \"%s\""), line) ;

  return 1 ;
}

static int setupp_readline(FILE *sf, setupp_liststr *member, omem *mem)
{
  /* Read a entry from the setup file */
  /* Return 1 for success, 0 for failure */

  //char line[temp_string_len] ;
  int ll = 0 ;
  int c ;

  /* Read a line */

  do
  {
    if (!omem_ensure(mem, ll+1)) return 0 ; /* Failed */

    if (feof(sf)) return 1 ;

    c = fgetc(sf) ;

    if ((c == EOF) ||
        (c == '\n'))
    {
      /* Found end of line / file */
      ((char *) mem->mem)[ll++] = '\n' ;
      if (!omem_ensure(mem, ll+1)) return 0 ; /* Failed */
      c = '\0' ;
    }

    ((char *) mem->mem)[ll++] = (char) c ;
  } while (c != '\0') ;

//  if (!fgets(line, temp_string_len, sf))
//  {
//    if (feof(sf)) return 1 ; /* It's OK - we reached the end of file */
//
//    werr(0, "Error whilst reading file") ;
//    return 0 ;
//  }

  /* Ignore comment lines */
  if (strchr(SETUPP_COMMENTS, ((char *) mem->mem)[0])) return 1 ;

  return setupp_sreadline((char *) mem->mem, member) ;
}

int setupp_swriteline(omem *mem, setupp_liststr *member, int index)
{
  /* Write an entry to the setup file line */

  char tps[SETUPP_MAXTAG+4] ;

  // Add suffix
  setupp_tag_with_suffix(member, tps, index) ;

  ((char *) mem->mem)[0] = '\0' ;

  /* What type is it? */

  switch (member->type) {
    case SETUPP_SPECIAL:
      if (member->parsew(temp_string, member, index))
      {
        if (omem_ensure(mem, strlen(tps) + 3 + strlen(temp_string)))
        sprintf((char *) mem->mem, "%s %s\n", tps, temp_string) ;
      }
      break ;

      case SETUPP_INT:
      sprintf((char *) mem->mem, "%s %d\n", tps, *((int *) member->data)) ;
      break ;

    case SETUPP_FLOAT:
      sprintf((char *) mem->mem, "%s %f\n", tps, *((float *) member->data)) ;
      break ;

    case SETUPP_STRING:
    {
      if (omem_ensure(mem, strlen(tps) + 3 +
                           strlen((char *) member->data)))
        sprintf((char *) mem->mem, "%s %s\n",
                         tps, (char *) member->data) ;
      break ;
    }

    case SETUPP_ASTRING:
      if (*(char *)member->data)
      {
        if (omem_ensure(mem, strlen(tps) + 3 +
                             strlen(*(char **) member->data)))
        sprintf((char *) mem->mem, "%s %s\n",
                         tps, *(char **) member->data) ;
      }
      else
      {
        sprintf((char *) mem->mem, "%s\n", tps) ;
      }
      break ;

      case SETUPP_ULONG:
      sprintf((char *) mem->mem, "%s %lu\n", tps,
        *((unsigned long *) member->data)) ;
      break ;

      case SETUPP_ULONGARR:
      {
        int i, ll = 0 ;

        ll = strlen(tps) ;
        if (omem_ensure(mem, ll + 3))
        {
          sprintf((char *) mem->mem, "%s", tps) ;
          for (i=0; i<member->data_len; ++i)
          {
            char string[32] ;
            sprintf(string, " %lu",
              ((unsigned long *) member->data)[i]) ;

            ll += strlen(string) ;

            if (omem_ensure(mem, ll+3))
              strcat((char *) mem->mem, string) ;
          }
          strcat((char *) mem->mem, "\n") ;
        }
        break ;
      }
  }

  return 1 ; /* OK */
}

static int setupp_writeline(FILE *sf, setupp_liststr *member, omem *mem, int index)
{
  /* Write an entry to the setup file */
  /* If member takes a suffix, try to write each one */

  if (!setupp_swriteline(mem, member, index)) return 0 ;
  return (fputs((char *) mem->mem, sf) != EOF) ;
}

static int setupp_writelist(FILE *sf, setupp_liststr *member)
{
  /* member points to start of list
     write list to setup file */

  omem *mem ;

  if (!member) return 0 ; /* Failed */

  /* Create buffer */
  mem = omem_new(128, 128) ;

  if (!mem)
  {
    werr(0, "Unable to allocate buffer") ;
    return 0 ;
  }

  while (member->data) /* End with NULL data */
  {
    int i ;

    // For each index value (some members take a suffix range)
    for (i=0; i < ((member->suffix_type == SETUPP_SUFFIX_NONE) ? 1 : member->index_max); ++i)
    {
      if (!setupp_writeline(sf, member, mem, i))
      {
        omem_wipe(mem) ;
        return 0 ; /* Failed */
      }
    }
    ++member ;
  }

  omem_wipe(mem) ;
  return 1 ; /* OK */
}

static int setupp_readlist(FILE *sf, setupp_liststr *member)
{
  /* member points to start of list
     read list from setup file */

  /* Return 1 for success, 0 for failure */

  omem *mem ;

  if (!member) return 0 ; /* Failed */

  /* Create buffer */
  mem = omem_new(128, 128) ;

  if (!mem)
  {
    werr(0, "Unable to allocate buffer") ;
    return 0 ;
  }

  while (!feof(sf))
    if (!setupp_readline(sf, member, mem))
    {
      omem_wipe(mem) ;
      return 0 ; /* Failed */
    }

  omem_wipe(mem) ;
  return 1 ; /* OK */
}

//int setupp_write_buffer(char *buffer, int buffsize,
//  setupp_liststr *member)
//{
//  /* member points to start of list
//     write list to buffer */
//
//  char line[256] ;
//  int written = 0 ;
//
//  if (!member) return 0 ; /* Failed */
//
//  while (member->data) /* End with NULL data */
//  {
//    int linelen ;
//
//    if (!setupp_swriteline(line, member)) return 0 ; /* Failed */
//
//    linelen = strlen(line) ;
//    if (written + linelen  > buffsize) return 0 ;
//
//    strcat(buffer, line) ;
//
//    written += linelen ;
//
//    ++member ;
//  }
//
//  return 1 ; /* OK */
//}
//
//int setupp_read_buffer(char *buffer, setupp_liststr *member)
//{
//  /* member points to start of list
//     read list from buffer (NULL terminated) */
//
//  /* NB: will corrupt buffer, breaking it up into separate strings */
//
//  char *line, *nl ;
//  int retval = 0 ;
//
//  if (!member) return 0 ; /* Failed */
//
//  line = buffer ;
//
//  do {
//    nl = strchr(line, '\n') ;
//    if (!nl) return retval ;
//    *nl = '\0' ;
//
//    if (!setupp_sreadline(line, member))
//      return 0 ; /* Failed */
//
//    retval = 1 ;
//
//    line = ++nl ;
//  } while (*line) ;
//
//  return retval ;
//}

int setupp_write(const char *name, setupp_liststr *list)
{
  FILE *sf ;
  
  werr(WERR_DEBUG0, "Save setup/devices/stats to %s", name);

  sf = fopen(name, "w") ;

  if (!sf)
  {
    werr(0, _("Error when saving setup to %s"), name) ;
    return 0 ; /* Failed */
  }

  setupp_writelist(sf, list) ;

  fclose(sf) ;
  
  werr(WERR_DEBUG0, "Setup/devices/stats saved to %s Ok", name);

  return 1 ;
}

int setupp_read(const char *name, setupp_liststr *list)
{
  FILE *sf ;
  int rv ;

  werr(WERR_DEBUG0, "Read setup/devices/stats from %s", name);

  sf = fopen(name, "r") ;
  if (!sf)
  {
    werr(WERR_WARNING, _("Error when loading setup from %s"), name) ;
    return 0 ; /* Failed */
  }

  rv = setupp_readlist(sf, list) ;

  fclose(sf) ;

  werr(WERR_DEBUG0, "Setup/devices/stats read from %s Ok", name);

  return rv ; /* Return 1 for success, 0 for failure */
}
