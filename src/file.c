/* file_ro.c */

/*
 * For !OWW project
 * One-wire weather
 * Dr. Simon J. Melhuish
 * 1999 - 2000
 * Free for non-comercial use
 * Dallas parts subject to their copyright and conditions
 *
 */

/* Linux specific file functions for Oww
   Simon J. Melhuish
   Tue 26th September 2000
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "werr.h"
#include "setup.h"
#include "intl.h"

//#include "message.h"

#ifndef NOGUI

int file_complete_save(void *handle)
{
  /* Completion of a drag save */


  return 0 ;
}
#endif

int file_create_dir(char *dirname)
{
  /*werr(0,"file_create_dir(%s)", dirname) ;*/

#ifdef WIN32
  return (-1 != mkdir(dirname)) ;
#else
  return (-1 != mkdir(dirname, 0755)) ;
#endif
}

int file_create_file(char *filename, int type)
{
  /* Try to create the file */
  int fd ;

  /* We don't use the RISC OS filetype */
  type = type ;

  /*werr(0,"file_create_file(%s, %d)", filename, type) ;*/

  fd = creat(filename, 0664) ;
  if (fd == -1) {
    /* Open failed */
    return 0 ;
  }
  close(fd) ;
  return 1 ;
}

int file_check_dir(char *dir_name, int create)
{
  /* Check that directory dir_name exists */

  struct stat statbuf ;

  /*werr(0, "file_check_dir(%s, %d)", dir_name, create) ;*/

  /* Check that directory exists */
  if (access(dir_name, F_OK)) {
    /* Directory does not exist */
    if (!create)
    {
      werr(WERR_WARNING, _("%s not found."), dir_name) ;
      return 0 ;
    }
    else
    {
      if (!file_create_dir(dir_name)) {
        werr(0, _("Failed to create %s"), dir_name) ;
        return 0 ;
      }
      return 1 ;
    }
  }

  /* dir_name exists, but is it a directory? */

  if (stat(dir_name, &statbuf)) {
    /* stat failed */
    werr(0, _("Unable to get directory information: %s"),
      strerror(errno)) ;
    return 0 ;
  }

  /* statbuf.st_mode tells us the mode */

  if (!S_ISDIR(statbuf.st_mode)) {
    /* dir_name is something other than a directory */
    werr(0, _("%s is not a directory"), dir_name) ;
    return 0 ;
  }

  /* Do we have write permission? */

  if (access(dir_name, W_OK)) {
    /* Directory does not give us write permission */
    werr(0, _("No write permission in %s"), dir_name) ;
    return 0 ;
  }

  return 1 ; /* OK */
}

int file_check_file(char *file_name, int create)
{
  /* Check that file file_name exists */

  struct stat statbuf ;

  /*werr(0, "file_check_file(%s, %d)", file_name, create) ;*/

  /* Check that file exists */
  if (access(file_name, F_OK)) {
    /* File does not exist */
    if (!create)
    {
      werr(WERR_WARNING, _("File %s not found"), file_name) ;
      return 0 ;
    }
    else
    {
      FILE *fp ;

      if (!file_create_file(file_name, 0xdfe)) {
        werr(0, _("Failed to create file %s"), file_name) ;
        return 0 ;
      }

      fp = fopen(file_name, "w") ;
      if (!fp) return 0 ;
      fprintf(fp, setup_format_loghead) ;
      fprintf(fp, "\n") ;
      fclose(fp) ;
      return 1 ;
    }
  }

  /* file_name exists, but is it a file? */

  if (stat(file_name, &statbuf)) {
    /* stat failed */
    werr(0, _("Unable to get file information: %s"),
      strerror(errno)) ;
    return 0 ;
  }

  /* statbuf.st_mode tells us the mode */

  if (!S_ISREG(statbuf.st_mode)) {
    /* file_name is something other than a directory */
    werr(0, _("%s is not a normal file"), file_name) ;
    return 0 ;
  }

  /* Do we have write permission? */

  if (access(file_name, W_OK)) {
    /* File does not give us write permission */
    werr(0, _("No write permission in %s"), file_name) ;
    return 0 ;
  }

  return 1 ; /* OK */
}
