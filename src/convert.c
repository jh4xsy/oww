/*
 * convert.c
 * For !OWW project
 * One-wire weather
 * Dr. Simon J. Melhuish
 * August - December 1999
 * Free for non-comercial use
 * Dallas parts subject to their copyright and conditions
 *
 */

#include <stdio.h>
#include "convert.h"
#include "werr.h"
#include "intl.h"
#include "globaldef.h"

/* General unit conversion function */

float
convert_unit(float val, int unit)
{
  switch(unit)
  {
    /* These are the "natural" units - no conversion */
    case convert_unit_c:
    case convert_unit_mph:
    case convert_unit_inches:
    case convert_unit_millibar:
    case convert_unit_none:
      return val ;

    case convert_unit_f:
      return (32.0F + 1.8F * val) ;

    case convert_unit_kph:
      return (val * 1.6093475F) ;

    case convert_unit_mps:
      return val * 0.44704F ;

    case convert_unit_knot:
      return val * 0.868976F ;

    case convert_unit_mm:
      if (val > 999.99F) val = 999.99F ;
      return (val * 25.4F) ;

    case convert_unit_inches_hg:
      return (val * 2.9529e-2F) ;

    case convert_unit_kpa:
      return (val * 0.1F) ;

    case convert_unit_atm:
      return (val * 0.000986923267F) ;

    default:
      werr(WERR_WARNING+WERR_AUTOCLOSE, "Bad unit to convert_unit()") ;
  }

  return val ;
}

char *
convert_unit_short_name(int unit)
{
  switch (unit)
  {
    case   convert_unit_none:
      return "" ;

    case convert_unit_c:
      return DEGSYMB "C" ;

    case convert_unit_f:
      return DEGSYMB "F" ;

    case convert_unit_kph:
      return _("km/h") ;

    case convert_unit_mph:
      return _("mph") ;

    case convert_unit_mps:
      return _("m/s") ;

    case convert_unit_knot:
      return _("knots") ;

    case convert_unit_inches:
      return "\"" ;

    case convert_unit_mm:
      return _("mm") ;

    case convert_unit_millibar:
      return _("mB") ;

    case convert_unit_inches_hg:
      return _("\"Hg") ;

    case convert_unit_kpa:
      return _("kPa") ;

    case convert_unit_atm:
      return _("ATM") ;

    default:
      return _("Bad unit type") ;
  }
}

float convert_temp(float c, int f)
{
  if (f) return (32.0F + 1.8F * c) ;
  return c ;
}

/** Convert speed (in mph) to another unit
*/
float convert_speed(float s, int m)
{
  switch (m)
  {
    case CONVERT_KPH:
      return s * 1.6093475F ;

    case CONVERT_MPH:
      return s ;

    case CONVERT_MPS:
      return s * 0.44704F ;

    case CONVERT_KNOT:
      return s * 0.868976F ;
  }
  //if (m) return s ;
  //return (s * 1.6093475F) ; /* Assuming Dallas cal. is to US miles! */

  return s ;
}

float convert_mm(float r, int mm)
{
  if (r > 999.99F) r = 999.99F ;
  if (mm) return (r * 25.4F) ;

  return r ;
}

/* wsstruct holds barom as mBar
   This can convert to inches of Hg */

float convert_barom(float b, int unit)
{
  switch(unit)
  {
    case CONVERT_MILLIBAR:
      return b ;

    case CONVERT_INCHES_HG:
      return b * 2.9529e-2F ;

    case CONVERT_KPA:
      return b * 0.1F ;

    case CONVERT_ATM:
      return b * 0.000986923267F ;
  }

  return b ;
}

char *convert_mm_string(float r, int mm, int comma)
{
  static char st[20] ;
  if (r > 999.99F) r = 999.99F ;
  if (comma) {
    if (mm) {
      sprintf(st, "%.1f, mm", (double) r * 25.4) ;
    } else {
      sprintf(st, "%.2f, %s", r, _("inches")) ;
    }
  } else {
    if (mm) {
      sprintf(st, "%.1f mm", (double) r * 25.4) ;
    } else {
      sprintf(st, "%.2f %s", r, _("inches")) ;
    }
  }

  return st ;
}
