/* interactive.c */

/* CLI for Oww */

/* Dr. Simon J. Melhuish */
/* 2002 - 2003 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

#include "interactive.h"
#include "setupp.h"
#include "setup.h"
#include "progstate.h"
#include "devices.h"
#include "werr.h"
#include "intl.h"
#include "weather.h"
#include "oww.h"
#include "applctn.h"

extern setupp_liststr setup_list[], device_list[] ;
//extern int setup_id_to_string(char *string, void *data) ;

//extern int ReadSensor_Values(float *Pressure, float *Temperature) ;
//extern int ReadSensor_Calibration(void) ;

extern int daemon_proc ; /* Running as daemon? */

# ifdef ENABLE_INTERACTIVE
static void
list_tags(setupp_liststr *member)
{
  omem *mem ;

  if (!member) return ;

  /* Create buffer */
  mem = omem_new(128, 128) ;

  if (!mem) return ;

  while (member->data) /* End with NULL data */
  {
    int i ;

    // For each index value (some members take a suffix range)
    for (i=0; i < ((member->suffix_type == SETUPP_SUFFIX_NONE) ? 1 : member->index_max); ++i)
    {
      if (!setupp_swriteline(mem, member, i))
      {
        omem_wipe(mem) ;
        return ;
      }
      printf("%s", (mem->mem==NULL)? "NULL":(char *) mem->mem) ;
    }
    ++member ;
  }

  omem_wipe(mem) ;
}

/* User wants to change device allocation */
void
assign_device_by_number(int search_entry, int device_number)
{
  char *new_alloc_name = NULL ;
  int device_index = 1, i ;
  unsigned char family ;

  family = devices_search_list[search_entry].id[0] ;

  if (device_number==0)
  {
    new_alloc_name = strdup(_("--None--")) ;
  }
  else
  {
    for (i=0; i<DEVICES_TOTAL; ++i)
    {
      if (!devices_match_family(&devices_list[i], family)) continue ;
      if (device_number == device_index++)
      {
        new_alloc_name = strdup(devices_list[i].menu_entry) ;
        break;
      }
    }
  }

  if (new_alloc_name==NULL)
  {
    werr(WERR_WARNING, "Device not found");
    return;
  }


  printf("Reallocate search item %d to \"%s\"\n",search_entry+1,new_alloc_name);

  /* Reallocate to this entry */
  devices_queue_realloc(search_entry, new_alloc_name, 1, NULL) ;
  free(new_alloc_name);
  return ;
}

/* Print list of available assignments for a device
  Return current assignment number (ordinal in this list, 1-based)
*/
static int print_assignment_menu(int search_entry)
{
  int i, devnum, list_index = 1, assignment =0 ;
  unsigned char family ;

  family = devices_search_list[search_entry].id[0] ;
  devnum = devices_search_list[search_entry].alloc ;


  /* Print menu according to devices_list entries that
      want this family */

  printf("% 3d: %c %s\n", 0,
    (devices_search_list[search_entry].alloc == -1) ? '*':' ',
    _("--None--")) ;

  for (i=0; i<DEVICES_TOTAL; ++i)
  {
    /* Right family? */
    //if (devices_list[i].family != family) continue ;
    if (!devices_match_family(&devices_list[i], family)) continue ;
    if (devices_search_list[search_entry].alloc == i)
      assignment = list_index ;
    printf("% 3d: %c %s\n", list_index,
      (devices_search_list[search_entry].alloc == i) ? '*':' ',
      devices_list[i].menu_entry) ;
    ++list_index ;
  }

  return assignment ;
}

static int assign_device(void)
{
  int i, search_entry, new_assignment, old_asignment=-1 ;

  if (!devices_search_list)
  {
    devices_clear_search_list() ;
    devices_build_search_list_from_devices(1) ;
  }

  i = 0 ;

  for (i=0; i<devices_known; ++i)
  {
    /*werr(WERR_DEBUG1, "Adding device %d to list", i) ;*/
    setup_id_to_string(temp_string, devices_search_list[i].id);
    printf("% 3d: %s %s\n", i+1, temp_string,
      (devices_search_list[i].alloc < 0) ?
      "" : devices_list[devices_search_list[i].alloc].menu_entry
    ) ;
  }

  //werr(WERR_DEBUG1, "Finished building list");

  printf(_("Select device for assignment: "));
  scanf("%d", &search_entry);

  if ((search_entry<1) || (search_entry>devices_known))
  {
    printf(_("%d is outside of the valid range\n"), search_entry);
    return 0;
  }

  --search_entry ; // Chage from 1-based to 0-based index

  setup_id_to_string(temp_string, devices_search_list[search_entry].id);
  /*printf(_("You selected device %d: %s %s\n"), search_entry+1, temp_string,
      (devices_search_list[search_entry].alloc < 0) ?
      "" : devices_list[devices_search_list[search_entry].alloc].menu_entry
    ) ;*/

  printf(_("Choose assignment from:\n"));
  old_asignment = print_assignment_menu(search_entry);

  scanf("%d", &new_assignment);

  if (new_assignment==old_asignment)
  {
    printf(_("Assignment unchanged\n"));
    return 0 ;
  }

  assign_device_by_number(search_entry, new_assignment);

  return 0 ;
}

static int set_cal(void)
{
  int i, listi, devchoice, cali ;

  if (!devices_search_list)
  {
    devices_clear_search_list() ;
    devices_build_search_list_from_devices(1) ;
  }

  i = listi = 0 ;

  for (i=0; i<DEVICES_TOTAL; ++i)
  {
    /* Skip unavailable devices */
    if (devices_list[i].id[0] == 0) continue ;

    /* Skip devices with no cal entries */
    if (devices_list[i].ncalib == 0) continue ;

    printf("% 3d: %s\n", ++listi, devices_list[i].menu_entry) ;
  }

  //werr(WERR_DEBUG1, "Finished building list");

  printf(_("Select device for cal setting: "));
  scanf("%d", &devchoice);

  if ((devchoice<1) || (devchoice>listi))
  {
    printf(_("%d is outside of the valid range\n"), devchoice);
    return 0;
  }

  listi = 0 ;

  for (i=0; i<DEVICES_TOTAL; ++i)
  {
    /* Skip unavailable devices */
    if (devices_list[i].id[0] == 0) continue ;

    /* Skip devices with no cal entries */
    if (devices_list[i].ncalib == 0) continue ;

    if (++listi == devchoice)
    {
      devchoice = i ;
      break ;
    }
  }

  printf(_("Choose cal entry from:\n"));
  for (i=0; i<devices_list[devchoice].ncalib; ++i)
  {
    printf("% 3d: %s \t= %g\n", i+1,
      devices_cal_name(devices_list[devchoice].devtype, i),
      devices_list[devchoice].calib[i]) ;
  }

  scanf("%d", &cali);

  if ((cali < 1) || (cali>listi))
  {
    printf(_("No change\n"));
    return 0 ;
  }

  --cali ;

  printf("%s = ? ",
    devices_cal_name(devices_list[devchoice].devtype, cali));

  scanf("%g",&(devices_list[devchoice].calib[cali])) ;

  return 0 ;
}

static int handle_special(void)
{
  int i, listi, devchoice, cali ;

  if (!devices_search_list)
  {
    devices_clear_search_list() ;
    devices_build_search_list_from_devices(1) ;
  }

  i = listi = 0 ;

  for (i=0; i<DEVICES_TOTAL; ++i)
  {
    /* Skip unavailable devices */
    if (devices_list[i].id[0] == 0) continue ;

    /* Skip devices with no special handler */
    if (devices_list[i].parser_special == NULL) continue ;

    printf("% 3d: %s\n", ++listi, devices_list[i].menu_entry) ;
  }

  //werr(WERR_DEBUG1, "Finished building list");

  printf(_("Select device for special action: "));
  scanf("%d", &devchoice);

  if ((devchoice<1) || (devchoice>listi))
  {
    printf(_("%d is outside of the valid range\n"), devchoice);
    return 0;
  }

  listi = 0 ;

  for (i=0; i<DEVICES_TOTAL; ++i)
  {
    /* Skip unavailable devices */
    if (devices_list[i].id[0] == 0) continue ;

    /* Skip devices with no special handler */
    if (devices_list[i].parser_special == NULL) continue ;

    if (++listi == devchoice)
    {
      devchoice = i ;
      break ;
    }
  }

  if (0 != devices_list[devchoice].parser_special(devchoice))
	printf("The command failed.\n");

  return 0 ;
}

# endif // ENABLE_INTERACTIVE

/* Parse a command line */
/* Return non-zero on failure */

int
interactive_parse(char *line)
{
# ifdef ENABLE_INTERACTIVE
  //printf("interactive_parse(\"%s\")\n", line) ;

  char *delim ;

  delim = &line[0];

  //Search for command delimiter: space or \n

  while ((*delim != '\0') && (!isspace(*delim))) ++delim ;

  if (*delim != '\0')
  {
    // Terminate at delimiter
    *delim++ = '\0' ;
    // Advance past delimiter
    while (isspace(*delim)) ++delim ;
  }
  // else left pointing at null

  //printf("line=\"%s\", delim=\"%s\"\n", line, delim);

  if (0 == strcmp(line, _("assign")))
  {
    assign_device();
  }

  else if (0 == strcmp(line, _("cal")))
  {
    set_cal();
  }

  else if (0 == strcmp(line, _("special")))
  {
    handle_special();
  }

  else if (0 == strcmp(line, _("daemon")))
  {
#ifdef NOGUI
    if (1 == daemon_proc)
      printf(_("Already running as a daemon\n"));
    else
      applctn_go_daemon();
#else
    printf(_("Run owwnogui to use daemon mode\n"));
#endif
  }

  else if(0 == strcmp(line, _("quit")))
  {
    exit(0);
  }

  else if (0 == strcmp(line, _("search")))
  {
    state_new(state_learn) ;
    printf(_("Searching...\n")) ;
  }

  else if (0 == strcmp(line, _("setup")))
  {
    setupp_sreadline(&line[6], setup_list) ;
    printf(_("Done\n")) ;
  }

  else if (0 == strcmp(line, _("devices")))
  {
    setupp_sreadline(&line[8], device_list) ;
    printf(_("Done\n")) ;
  }

  else if (0 == strcmp(line, _("reset")))
  {
    weather_reset_stats() ;
    printf(_("Done\n")) ;
  }

  else if (0 == strcmp(line, _("save")))
  {
    if (0 == strcmp(delim, _("setup")))
    {
      setup_save_setup(0) ;
      printf(_("Saved setup\n")) ;
    }
    else if (0 == strcmp(delim, _("setupng")))
    {
      setup_save_setup(1) ;
      printf(_("Saved setupNG\n")) ;
    }
    else if (0 == strcmp(delim, _("devices")))
    {
      setup_save_devices() ;
      printf(_("Saved devices\n")) ;
    }
    else if (0 == strcmp(delim, _("stats")))
    {
      setup_save_stats() ;
      printf(_("Saved stats\n")) ;
    }
    else
    {
      printf(_("Unrecognized save option\n")) ;
    }
  }

  else if (0 == strcmp(line, _("wipe")))
  {
    weather_reset_stats() ;
    printf(_("Done\n")) ;
  }

  else if (0 == strcmp(line, "?"))
  {
    if (0 == strcmp(delim, _("setup")))
      list_tags(setup_list) ;
    else if (0 == strcmp(delim, _("devices")))
      list_tags(device_list) ;
    else
      printf(_("Available commands:\n"
    	  "\tassign\n"
    	  "\tcal\n"
    	  "\tdaemon\n"
    	  "\tdevices [devices line]\n"
    	  "\tquit\n"
    	  "\treset\n"
    	  "\tsave [setup | setupng | stats | devices]\n"
    	  "\tsearch\n"
    	  "\tsetup [setup line]\n"
    	  "\tspecial\n"
    	  "\twipe\n"
    	  "\t? devices\n"
    	  "\t? setup\n") );
  }
  else
  {
    printf(_("Unrecognized command: \"%s\"\n"), line) ;
  }

# endif // ENABLE_INTERACTIVE
  return 0 ; /* Ok */
}
