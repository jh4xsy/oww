#ifndef TAI8590_H
#define TAI8590_H 1

void tai8590_New(devices_struct *devp) ;

void *tai8590_message(wsstruct *wd, int *len) ;

int tai8590_SendMessage(int dev, const char *message, int clear) ;

int tai8590_ClearDisplay(int dev) ;

#endif
