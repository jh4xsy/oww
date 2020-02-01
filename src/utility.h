/* utility.h */

/* various functions */

#ifndef HAVE_STRDUP
char *strdup(const char *s) ;
#endif

#ifdef RISCOS
int setenv(const char *, const char *, int ) ;
#endif

#ifdef WIN32
int setenv(const char *, const char *, int ) ;
unsigned long int csGettick(void);
#endif

// #ifndef HAVE_STRTOF
// extern float strtof (const char *nptr, char **endptr) ;
// #endif
