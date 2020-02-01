/* auxwin_t.h */

/* Private deinitions shared by auxwin files */

/* Type for a field */

typedef struct auxwin_field_ {
  float val ;  /* value */
  int prec ;   /* Precision */
  float min ;
  float max ;
  char *name ;
  int index ;
  struct auxwin_field_ *prev ;
  struct auxwin_field_ *next ;
  int id ;
} auxwin_field ;

enum auxwin_fields {
  Auxwin_Vane_Speed,
  Auxwin_Vane_Bearing,
  Auxwin_Rain,
  Auxwin_Rain_Rate,
  Auxwin_T,
  Auxwin_RH,
  Auxwin_TRH,
  Auxwin_TDP,
  Auxwin_BP,
  Auxwin_GPC
} ;
