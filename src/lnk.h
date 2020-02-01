#ifndef XXXlnk_h
#define XXXlnk_h


// lnk level definitions
//   win32lnk.c
//   linuxlnk.c
//   blocklnk.c
//   etc

// exportable functions
int  OpenCOM(int, char *);
void CloseCOM(int);
void FlushCOM(int);
int  WriteCOM(int, int, uchar *);
int  ReadCOM(int, int, uchar *, int *);
void BreakCOM(int);
void SetBaudCOM(int, uchar);
void msDelay(int);
//int64_t msGettick(void);


#endif
