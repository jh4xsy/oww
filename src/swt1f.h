/* Functions for handling DS2409 branches */

extern int SetSwitch1F(int,uchar *,int,int,uchar *,int);
extern int SwitchStateToString1F(int, char *);
extern int FindBranchDevice(int,uchar *,uchar BranchSN[][8],int,int);
extern int owBranchFirst(int,uchar *,int,int);
extern int owBranchNext(int,uchar *,int,int);

//~ #define SWT1F_AllLinesOff     0
//~ #define SWT1F_DirectOnMain    1
//~ #define SWT1F_SmartOnAux      2
//~ #define SWT1F_StatusReadWrite 3
//~ #define SWT1F_SmartOnMain     4

enum {
  SWT1F_AllLinesOff = 0,
  SWT1F_DirectOnMain,
  SWT1F_SmartOnAux,
  SWT1F_StatusReadWrite,
  SWT1F_SmartOnMain,
  SWT1F_DischargeLines
} ;
