#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  FILE *ini, *dev ;
  char buffer[20] ;
  int i ;
  char tags[][10] = {
    "wv0 " ,
    "wv1 " ,
    "wv2 " ,
    "wv3 " ,
    "wv4 " ,
    "wv5 " ,
    "wv6 " ,
    "wv7 " ,
    "anem " ,
    "vane " ,
    "T1 " ,
    "offset "
  } ;

  if (argc != 3)
  {
    fprintf(stderr, "Usage: ini2dev ini_file devices_file\n") ;
    fprintf(stderr, "  e.g  [Linux]   ini2dev ini.txt .oww_devices\n") ;
    fprintf(stderr, "  or   [RISC OS] ini2dev ini/txt <choices$dir>.Oww.Devices\n\n") ;
    exit(0) ;
  }

  ini = fopen(argv[1], "r") ;
  if (!ini)
  {
    fprintf(stderr, "%s failed to open\n\n", argv[1]) ;
    exit(1) ;
  }

  dev = fopen(argv[2], "w") ;
  if (!dev)
  {
    fprintf(stderr, "%s failed to open\n\n", argv[2]) ;
    exit(1) ;
  }

  for (i=0; i<11; ++i)
  {
    /* Get line from ini file */
    fgets(buffer, 19, ini) ;

    /* Terminate after 16-character ID */
    buffer[16] = '\0' ;

    fprintf(dev, "%s%s\n", tags[i], buffer) ;
    printf("Wrote: %s%s\n", tags[i], buffer) ;
  }

  fprintf(dev, "%s0\n", tags[11]) ;
  printf("Wrote: %s0\n", tags[11]) ;

  fclose(dev) ;
  fclose(ini) ;

  exit(0) ;
}
