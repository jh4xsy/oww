/*
  choices.h

  Find out where to read / write setup and devices files

*/

extern
char *choices_stats_read,
     *choices_stats_write,
     *choices_setup_read,
     *choices_setup_write,
     *choices_setupng_read,
     *choices_setupng_write,
     *choices_values_read,
     *choices_values_write,
     *choices_devices_read,
     *choices_devices_write ;

void choices_find_files(int argc, char *argv[]) ;
