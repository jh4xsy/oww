/* mainwin.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <unistd.h>
#include <assert.h>

/*#include "gtkTrace.h"*/

#include "wstypes.h"
#include "setup.h"
#include "oww.h"
#include "convert.h"
//#include "message.h"
#include "globaldef.h"
#include "werr.h"
#include "weather.h"
#include "meteo.h"
#include "intl.h"
#include "config.h"
#include "mainwin.h"

int mainwin_kill_anim = 0 ;

//extern wsstruct ws ;
extern time_t the_time ;
//extern unsigned int next_animate ;

int
devices_have_rain(void) ;

/* Prototype for source name function */
char *progstate_sourcename(void) ;

/*extern GtkWidget *mainwin_id ;*/

extern GtkWidget *drawing_area, *mainwin_id ;

static GdkFont *font = NULL ;

static GdkGC *penBlack = NULL;
static GdkGC *penFatBlack = NULL;
static GdkGC *penBlue = NULL;
static GdkGC *penRed = NULL;
static GdkGC *penGreen = NULL;
static GdkGC *penFatGreen = NULL;
static GdkGC *penGray = NULL;
static GdkGC *penDarkGray = NULL;
static GdkGC *penWhite = NULL;
static GdkGC *penFatWhite = NULL;
static GdkGC *penYellow = NULL;
static GdkGC *penOrange = NULL;
static GdkGC *penViolet = NULL;

static GdkGC *VaneArc[16] ;

static GdkPixbuf *pixbuf_top[3] = {NULL, NULL, NULL} ;
static GdkPixbuf *pixbuf_body[1] = {NULL} ;
static GdkPixbuf *pixbuf_bottom[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL} ;
static GdkPixbuf *pixbuf_rh[1] = {NULL} ;

static int anim_top_frame = 0 , anim_bottom_frame = 0 ;

/* The graphics */

extern GdkPixbuf *pixbuf_top1 ;

/* Functions to manipulate the main display window */

static GdkPixmap *pixmap = NULL ;

/* Useful func from choices_linux.c */

char *path_and_leaf(const char *s1, const char *s2) ;

static int mainwin_load_graphics(const char *filename[], GdkPixbuf *pixbuf[],
  int a, int b)
{
  char *filepath ;
  int i ;
  GError *err = NULL; 

  for (i=a; i<=b; ++i) {
    filepath = path_and_leaf("pixmaps", filename[i]) ;
    if (access(filepath, R_OK)) /* Not able to read? */
    {
      filepath = path_and_leaf(PACKAGE_PIXMAPS_DIR, filename[i]) ;
      if (access(filepath, R_OK)) /* Still not able to read? */
      {
        werr(0, "Unable to find %s", filename[i]) ;
        return 0 ;
      }
    }

    if (!pixbuf[i]) pixbuf[i] = gdk_pixbuf_new_from_file(filepath, &err) ;
    if (err != NULL) 
    {
      g_warning("Found %s, but could not load it. Is the latest gdk-pixbuf installed?", filename[i]) ;
      g_error_free (err);
    }
    free(filepath) ;
  }

  return 1 ;
}

static void mainwin_unload_graphics(GdkPixbuf *pixbuf[], int a, int b)
{
  int i ;

  for (i=a; i<=b; ++i) {
    if (pixbuf[i]) {
      gdk_pixbuf_unref(pixbuf[i]) ;
      pixbuf[i] = NULL ;
    }
  }
}

static void ensure_graphics(void)
{
  const char *graphics_top[3] = {"top1.jpg", "top2.jpg", "top3.jpg"} ;
  const char *graphics_body[1] = {"body.jpg"} ;
  const char *graphics_bottom[8] = {
    "bottom1.jpg",
    "bottom2.jpg",
    "bottom3.jpg",
    "bottom4.jpg",
    "bottom5.jpg",
    "bottom6.jpg",
    "bottom7.jpg",
    "bottom8.jpg"
  } ;
  const char *graphics_rh[1] = {"rh.png"} ;

  enum _grld {LOADED_NONE=0, LOADED_BASE, LOADED_ANIM} ;

  static int loaded = LOADED_NONE ;

  if (!setup_anim && (loaded == LOADED_BASE)) return ;
  if (setup_anim && (loaded == LOADED_ANIM)) return ;

  if (!setup_anim && (loaded == LOADED_ANIM)) {
    /* Dump anim graphics */
    mainwin_unload_graphics(pixbuf_top, 1, 2) ;
    mainwin_unload_graphics(pixbuf_bottom, 1, 7) ;
    loaded = LOADED_BASE ;
    return ;
  }

  if (!setup_anim && (loaded == LOADED_NONE)) {
    /* Load base graphics */
    mainwin_load_graphics(graphics_top, pixbuf_top, 0, 0) ;
    mainwin_load_graphics(graphics_body, pixbuf_body, 0, 0) ;
    mainwin_load_graphics(graphics_bottom, pixbuf_bottom, 0, 0) ;
    mainwin_load_graphics(graphics_rh, pixbuf_rh, 0, 0) ;
    loaded = LOADED_BASE ;
    return ;
  }

  if (setup_anim && (loaded == LOADED_NONE)) {
    /* Load all anim graphics */
    mainwin_load_graphics(graphics_top, pixbuf_top, 0, 2) ;
    mainwin_load_graphics(graphics_body, pixbuf_body, 0, 0) ;
    mainwin_load_graphics(graphics_bottom, pixbuf_bottom, 0, 7) ;
    mainwin_load_graphics(graphics_rh, pixbuf_rh, 0, 0) ;
    loaded = LOADED_ANIM ;
    return ;
  }

  if (setup_anim && (loaded == LOADED_BASE)) {
    /* Load all anim graphics */
    mainwin_load_graphics(graphics_top, pixbuf_top, 1, 2) ;
    mainwin_load_graphics(graphics_bottom, pixbuf_bottom, 1, 7) ;
    loaded = LOADED_ANIM ;
    return ;
  }
}

static GdkGC *GetPen (int nRed, int nGreen, int nBlue)
{
    GdkColor *c;
    GdkGC *gc;

    c = (GdkColor *) g_malloc (sizeof (GdkColor));
    c->red = nRed;
    c->green = nGreen;
    c->blue = nBlue;

    gdk_color_alloc (gdk_colormap_get_system (), c);

    gc = gdk_gc_new (pixmap);

    gdk_gc_set_foreground (gc, c);

    free(c);

    if (!gc) g_error("Failed to allocated colour %d,%d,%d",
      nRed, nGreen, nBlue) ;

    return (gc);
}

static void mainwin_init_colours (void)
{
  int i /*,
             r[16] = {0xffff, 0xeeee, 0xdddd, 0xcccc,
	              0xbbbb, 0xaaaa, 0x9999, 0x8888,
		      0x7777, 0x6666, 0x5555, 0x4444,
		      0x3333, 0x2222, 0x1111, 0x0000},
             g[16] = {0xffff, 0xeeee, 0xdddd, 0xcccc,
	              0xbbbb, 0xaaaa, 0x9999, 0x8888,
		      0x7777, 0x6666, 0x5555, 0x4444,
		      0x3333, 0x2222, 0x1111, 0x0000},
             b[16] = {0xffff, 0xeeee, 0xdddd, 0xcccc,
	              0xbbbb, 0xaaaa, 0x9999, 0x8888,
		      0x7777, 0x6666, 0x5555, 0x4444,
		      0x3333, 0x2222, 0x1111, 0x0000}*/ ;


             /*r[16] = {25, 50, 75, 100,
                      100, 100, 100, 100,
                      100, 100, 100, 100,
                      100, 100, 100, 100},
             g[16] = {0, 0, 0, 0,
                      20, 40, 60, 80,
                      100, 100, 100, 100,
                      100, 100, 100, 100},
             b[16] = {0, 0, 0, 0,
                      0, 0, 0, 0,
                      0, 15, 30, 45,
                      60, 75, 90, 100} ;*/

    /* --- Create pens for drawing --- */
    penBlack = GetPen (0, 0, 0);
    penFatBlack = GetPen (0, 0, 0);
    gdk_gc_set_line_attributes(penFatBlack, 3,
      GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER) ;
    penRed = GetPen (0xffff, 0, 0);
    penGreen = GetPen (0, 0xffff, 0);
    penFatGreen = GetPen (0, 0xffff, 0);
    penBlue = GetPen (0, 0x6000, 0xffff);
    penGray = GetPen (0xafff, 0xafff, 0xafff);
    penDarkGray = GetPen (0x6fff, 0x6fff, 0x6fff);
    penYellow = GetPen (0xffff, 0xffff, 0x0000);
    penOrange = GetPen (0xffff, 0x99ff, 0x0000);
    penViolet = GetPen (0x7fff, 0, 0xffff);
    penWhite = GetPen (0xffff, 0xffff, 0xffff);
    penFatWhite = GetPen (0xffff, 0xffff, 0xffff);

  for (i=0; i<16; ++i)
  {
    /*int r, g ;

    r = 0x3fff + i * 0x1b6d ;
    if (r > 0xffff)
    {
      g = r - 0xffff ;
      r = 0xffff ;
    }
    else
    {
      g = 0 ;
    }*/

    /*r = (i>7) ? 0xffff : 0x8000 + i * 0x1fff ;
    g = (i<=7) ? 0 : (i-7) * 0x1fff ;*/

    //printf("%3d %3x %3x %3x\n", i, r[i], g[i], b[i]) ;

    //VaneArc[i] = GetPen ((0xffff * r[i])/100, (0xffff * g[i])/100, (0xffff * b[i])/100);
    VaneArc[i] = GetPen (
      0x101 * (0xff & setup_ring_colours[i] >> 24),
      0x101 * (0xff & setup_ring_colours[i] >> 16),
      0x101 * (0xff & setup_ring_colours[i] >>  8)
    /*r[i], g[i], b[i]*/);
    g_assert(VaneArc[i] != NULL) ;
    gdk_gc_set_line_attributes(VaneArc[i], 10,
      GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL) ;
  }
}


/*gint repaint(gpointer data)
{
}*/

gint configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
  /*drawing_area = widget ;*/

  /* Discard any old pixmap */
  if (pixmap) gdk_pixmap_unref(pixmap) ;

  /* Create new pixmap the right size for the drawing area */
  
  g_assert(drawing_area != NULL) ;
  g_assert(drawing_area->window != NULL) ;

  pixmap = gdk_pixmap_new(drawing_area->window,
    drawing_area->allocation.width, drawing_area->allocation.height, -1) ;
  
  g_assert(pixmap != NULL) ;

  return TRUE ;
}

gint expose_event(GtkWidget *widget, GdkEventExpose *event)
{
  /* Copy pixmap to the drawing area */
  if (!pixmap) {
    werr(WERR_DEBUG0, "expose_event - no pixmap - creating\n");
    configure_event(widget, NULL) ;
  }

  gdk_draw_pixmap(widget->window,
    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
    pixmap,
    event->area.x, event->area.y,
    event->area.x, event->area.y,
    event->area.width, event->area.height) ;

  return FALSE ;
}

//gboolean
//draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
//{
//  guint width, height;
//  GdkRGBA color;
//
//  /* Copy pixmap to the drawing area */
//  if (!pixmap) {
//    werr(WERR_DEBUG0, "expose_event - no pixmap - creating\n");
//    configure_event(widget, NULL) ;
//  }
//
//  gdk_draw_pixmap(widget->window,
//    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
//    pixmap,
//    event->area.x, event->area.y,
//    event->area.x, event->area.y,
//    event->area.width, event->area.height) ;
//
//  width = gtk_widget_get_allocated_width (widget);
//  height = gtk_widget_get_allocated_height (widget);
//  cairo_arc (cr,
//             width / 2.0, height / 2.0,
//             MIN (width, height) / 2.0,
//             0, 2 * G_PI);
//
//  gtk_style_context_get_color (gtk_widget_get_style_context (widget),
//                               0,
//                               &color);
//  gdk_cairo_set_source_rgba (cr, &color);
//
//  cairo_fill (cr);
//
// return FALSE;
//}

static void rotate_points(int vane, GdkPoint *point, int offx, int offy, int N)
{
  /* Rotate coordinates by the given vane position, and adds offset */

  GdkPoint out ;

  long int sinval[] = {0,
    0,
    25080,
    46341,
    60547,
    65536,
    60547,
    46341,
    25080,
    0,
    -25080,
    -46341,
    -60547,
    -65536,
    -60547,
    -46341,
    -25080
   } ;

  long int cosval[] = {0,
    -65536,
    -60547,
    -46341,
    -25080,
    0,
    25080,
    46341,
    60547,
    65536,
    60547,
    46341,
    25080,
    0,
    -25080,
    -46341,
    -60547
   } ;

   int i ;
   long int out_x, out_y ;

   for (i=0; i<N; ++i) {

     out_x = (long int) point[i].x * cosval[vane] + (long int) point[i].y * sinval[vane] ;
     out.x = (int) offx + (out_x >> 16) ;
     out_y = (long int) point[i].x * -sinval[vane] + (long int) point[i].y * cosval[vane] ;
     out.y = (int) offy +  (out_y >> 16) ;
     /*printf("%d,%d -> %d,%d\n", point[i].x,point[i].y,out.x,out.y);*/
     point[i] = out ;
   }

}

static void draw_string_centred(gchar *string, GdkGC *style, gint x, gint y)
{
  gdk_draw_string(pixmap, font, style,
    x - gdk_string_width(font, string)/2,
    y + gdk_string_height(font, string)/2, string) ;
}

static void
mainwin_draw_vane_ring(int new_data, int vane, int xc, int yc, int radius)
{
  /* Keeping a record of vane positions, draw a grey-scale ring */

  int i ;
  static int vp[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
             pc[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} ;
  static int j = 0 ;

  if (new_data)
  {
    if ((vane < 1) || (vane > 16))
    {
      werr(WERR_DEBUG0, "Bad vane value: %d", vane) ;
      return ;
    }

    /* vane runs from 1 to 16 */
    /* Decrement count for old vane position */
    if ((vp[j] > 0) && (pc[vp[j]-1] > 0)) --pc[vp[j]-1] ;

    /* Note this position */
    vp[j] = vane ;

    /* Increment count for this position */
    ++pc[vane-1] ;

    /* Increment round-robin index */
    j = (j+1)%15 ;
  }

  /* Now draw the arcs for each position */
  for (i=0; i<16; ++i)
  {
    /* Unfilled arcs */
    gdk_draw_arc(pixmap, VaneArc[pc[i]], 1,
      xc-radius, yc-radius, 2*radius, 2*radius, (4 - i) * 360 * 4 - 720, 1440) ;
  }
}

static void mainwin_draw_compass_rose(int new_data, int vane, GdkRectangle area)
{
  #define BOSS_RAD 18

  gint xc, yc, i ;

  if (!devices_have_vane()) return;

  /*rotate_points_reset() ;*/

  xc = area.x + area.width/2 ;
  yc = area.y + area.height/2 ;

  /* Vane Arcs */
  mainwin_draw_vane_ring(new_data, vane, xc, yc, BOSS_RAD+7) ;

  /* Draw central boss */

  /* Filled circle */
  gdk_draw_arc(pixmap, penWhite, 1,
    xc-BOSS_RAD, yc-BOSS_RAD, 2*BOSS_RAD, 2*BOSS_RAD, 0, 360 * 64) ;

  /* Set line attributes */
  gdk_gc_set_line_attributes(penFatGreen, 8,
    GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;
  gdk_gc_set_line_attributes(penFatWhite, 8,
    GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;

  /* Text labels */

  for (i=0; i<8; ++i) {
    gchar *label[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"} ;
    gint pos ;
    GdkPoint tp = {0, 67} ;
    if (i%2) tp.y -= 4 ;

    pos = 1 + 2*i ;
    rotate_points(pos, &tp, xc, yc, 1) ;
    draw_string_centred(label[i], (pos==vane)? penGreen : penWhite,
      tp.x, tp.y) ;
  }

  /* Cardinal points - fat rounded lines */
  for (i=1; i<17; i+=4) {
    GdkPoint line[2] = {{0,29}, {0,51}} ;

    rotate_points(i, line, xc, yc, 2) ;
     gdk_draw_lines(pixmap, (i==vane)? penFatGreen : penFatWhite, line, 2) ;
   }

  /* Set line attributes */
  gdk_gc_set_line_attributes(penFatGreen, 12,
    GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;
  gdk_gc_set_line_attributes(penFatWhite, 12,
    GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;

  /* NE, SE, SW, NW - fat points */
  for (i=3; i<17; i+=4) {
    GdkPoint points[2] = {{0,33}, {0,33}} ;

    rotate_points(i, points, xc, yc, 2) ;
     gdk_draw_lines(pixmap, (i==vane)? penFatGreen : penFatWhite, points, 2) ;
   }

  /* Set line attributes */
  gdk_gc_set_line_attributes(penFatGreen, 7,
    GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;
  gdk_gc_set_line_attributes(penFatWhite, 7,
    GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;

  /* minor points */
  for (i=2; i<17; i+=2) {
    GdkPoint points[2] = {{0,31}, {0,31}} ;

    rotate_points(i, points, xc, yc, 2) ;
     gdk_draw_lines(pixmap, (i==vane)? penFatGreen : penFatWhite, points, 2) ;
   }

  /* Pointer */
  if (vane) {
    GdkPoint stem[] = {{0,BOSS_RAD}, {0,-BOSS_RAD}} ;
    GdkPoint head[] = {{-8,10}, {0,18}, {8,10}} ;

    rotate_points(vane, stem, xc, yc, 2) ;
    rotate_points(vane, head, xc, yc, 3) ;

    gdk_draw_lines(pixmap, penFatBlack, stem, 2) ;
    gdk_draw_polygon(pixmap, penBlack, 1, head, 3) ;
  }
}

static void mainwin_draw_thermometer(void)
{
  /* Draw the red bulb, red and white stem of the thermometer */

  int new_height ; /* Height of the red part of the stem */
  static double tmax = -1.0, tmin = 1.0 ;

  if (tmin > tmax) {
    tmax = (double) setup_tmax ;
    tmin = (double) setup_tmin ;
  }

  if (weather_primary_T(NULL) > tmax) tmax = weather_primary_T(NULL) ;
  if (weather_primary_T(NULL) < tmin) tmin = weather_primary_T(NULL) ;
  if (tmax == tmin) {
    tmin -= 0.1 ;
    tmax += 0.1 ;
  }

  if (weather_primary_T(NULL) >= tmax) {
    new_height = TSTEM_HEIGHT ;
  } else {
    if (weather_primary_T(NULL) <= tmin) {
      new_height = 0 ;
    } else {
      new_height = (int) floor(0.5 + TSTEM_HEIGHT *
        (weather_primary_T(NULL) - tmin) / (tmax - tmin)) ;
    }
  }

  /* Set line attributes */
  gdk_gc_set_line_attributes(penFatWhite, TSTEM_WIDTH,
    GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;

  /* Paint the white part of the stem */

  gdk_draw_line (pixmap, penFatWhite, TBULB_X0, TBULB_Y0+1-TBULB_RAD,
    TBULB_X0, TBULB_Y0-TBULB_RAD-TSTEM_HEIGHT) ;

  /* Paint the bulb */

  gdk_draw_arc(pixmap, penWhite, 0, TBULB_X0-TBULB_RAD-1, TBULB_Y0-TBULB_RAD-1,
     TBULB_RAD*2+1, TBULB_RAD*2+1, 0, 360 * 64) ;
  gdk_draw_arc(pixmap, penRed, 1, TBULB_X0-TBULB_RAD, TBULB_Y0-TBULB_RAD,
     TBULB_RAD*2-1, TBULB_RAD*2-1, 0, 360 * 64) ;

  /* Paint the red part of the stem */

  gdk_draw_rectangle (pixmap, penRed, TRUE, TBULB_X0+1-TSTEM_WIDTH/2,
    TBULB_Y0+1-TBULB_RAD-new_height, TSTEM_WIDTH-2, new_height) ;


}

static void mainwin_draw_solar(float f)
{
  /* Draw the Sun */

  gdk_draw_arc(pixmap, penRed, 1, SUN_X0-SUN_RAD, SUN_Y0-SUN_RAD,
     SUN_RAD*2-1, SUN_RAD*2-1, 0, 360 * 64) ;

  gdk_draw_arc(pixmap, penOrange, 1, SUN_X0-SUN_RAD, SUN_Y0-SUN_RAD,
     SUN_RAD*2-1, SUN_RAD*2-1, 5760, (int) (-23040.0 * f/setup_smax)) ;
}

static void mainwin_draw_uvi(float f)
{
  /* Draw the UVI warning box */

  GdkGC *pen;

  if (f < 2.5F)
	pen = penGreen ;
  else if (f < 5.5)
	pen = penYellow;
  else if (f < 7.5)
	pen = penOrange;
  else if (f < 10.5)
	pen = penRed;
  else
	pen = penViolet;

  gdk_draw_rectangle(pixmap, pen, TRUE, UVI_X0, UVI_Y0, UVI_WIDTH, UVI_HEIGHT);

//  gdk_draw_rectangle(pixmap, penWhite, FALSE, UVI_X0, UVI_Y0, UVI_WIDTH, UVI_HEIGHT);

//  gdk_draw_arc(pixmap, penRed, 1, SUN_X0-SUN_RAD, SUN_Y0-SUN_RAD,
//     SUN_RAD*2-1, SUN_RAD*2-1, 0, 360 * 64) ;
//
//  gdk_draw_arc(pixmap, penOrange, 1, SUN_X0-SUN_RAD, SUN_Y0-SUN_RAD,
//     SUN_RAD*2-1, SUN_RAD*2-1, 5760, (int) (-23040.0 * f/setup_smax)) ;
}

void
mainwin_update(int new_data)
{
  GdkRectangle update_rect ;
  static char date[128] = "xx/xx/xx xx:xx:xx ",
    wind_speed[128] = "xxxx km/h",
    rain[128] = "Rain  ",
    rainrate[128] = "(xxxx inches/hour)   " ;
  static char *old_source = NULL ;
  GdkRectangle comp = {300, 40 /*64*/, 150, 150} ;
  static int rh, last_rh = 0, bp, last_bp = 0 ;
  int sol, uv ;

  if (!pixmap) {
    werr(WERR_DEBUG0, "repaint - no pixmap yet!\n") ;
    return ;
  }

  /* Check if window title needs update */
  if (!old_source || strcmp(old_source, progstate_sourcename()))
  {
    /* Not equal */
    if (old_source) free(old_source) ;
    old_source = strdup(progstate_sourcename()) ;
    strcpy(temp_string, "Oww - ") ;
    strncpy(&temp_string[6], old_source, 40) ;
    gtk_window_set_title(GTK_WINDOW(mainwin_id), temp_string) ;
  }

/*gdk_draw_rectangle (pixmap, penGray, TRUE, 0, 0,
    drawing_area->allocation.width, drawing_area->allocation.height) ;*/

  if (!setup_anim) {
    anim_top_frame = 0 ;
    anim_bottom_frame = 0 ;
  } else {
    anim_bottom_frame = (ws.vane_bearing-1)/2 ;
  }

  ensure_graphics() ;

  gdk_pixbuf_render_to_drawable(pixbuf_top[anim_top_frame], pixmap,penBlack,
    0, 0, 0, 0, 474, 134, GDK_RGB_DITHER_NORMAL, 0, 0) ;
  gdk_pixbuf_render_to_drawable(pixbuf_body[0], pixmap,penBlack,
    0, 0, 0, 134, 474, 62, GDK_RGB_DITHER_NORMAL, 0, 0) ;
  gdk_pixbuf_render_to_drawable(pixbuf_bottom[anim_bottom_frame],
    pixmap,penBlack,
    0, 0, 0, 196, 474, 205, GDK_RGB_DITHER_NORMAL, 0, 0) ;

  /* Update the main window */

  strftime(date, 18, setup_displaydate, localtime(&the_time)) ;
  gdk_draw_string(pixmap, font, penWhite,
    272-gdk_string_width(font,date), 24, date) ;

  if (weather_primary_T(NULL) != UNDEFINED_T)
  {
    mainwin_draw_thermometer() ;

    sprintf(temp_string2, "%4.1f%s",
            convert_temp(weather_primary_T(NULL),
            setup_f),
            ((setup_f) ? "\xb0\x46" : "\xb0\x43")) ;
    gdk_draw_string(pixmap,
                    font,
                    penWhite,
                    104-gdk_string_width(font,temp_string2),
                    128,
                    temp_string2) ;
  }

  sol = weather_primary_solar(NULL) ;
  if (sol >= 0)
  {
    assert(sol<MAXSOL);
    mainwin_draw_solar(ws.solar[sol]) ;
  }
  
  uv = weather_primary_uv(NULL);
  if (uv >= 0)
  {
	assert(uv<MAXUV);
	mainwin_draw_uvi(ws.uvi[uv]);
  }

  /* Fill in RH values */
  rh = weather_primary_rh(NULL) ;

  if (rh >= 0)
  {
    /* We have an RH sensor */
	assert(rh<MAXHUMS);
    gdk_pixbuf_render_to_drawable_alpha(pixbuf_rh[0], pixmap,
    0, 0, 420, 176, 50, 51, GDK_PIXBUF_ALPHA_BILEVEL, 1,
    GDK_RGB_DITHER_NORMAL, 0, 0) ;

    /* RH */
    sprintf(temp_string2,
            "%4.1f %%",
            ws.RH[rh]) ;
    gdk_draw_string(pixmap,
                    font,
                    penWhite,
                    416-gdk_string_width(font,temp_string2),
                    228,
                    temp_string2) ;

    /* Td */
    gdk_draw_string(pixmap,
                    font,
                    penWhite,
                    306,
                    262,
                    "Td") ;

    sprintf(temp_string2,
            "%4.1f",
            convert_temp(meteo_dew_point(ws.Trh[rh], ws.RH[rh]), setup_f)) ;
    gdk_draw_string(pixmap,
                    font,
                    penWhite,
                    /*356*/ 426-gdk_string_width(font,temp_string2),
                    262,
                    temp_string2) ;
    gdk_draw_string(pixmap,
                    font,
                    penWhite,
                    428,
                    262,
                    (setup_f) ? "\xb0\x46" : "\xb0\x43") ;

    /* Trh */
    if (!setup_report_Trh1)
    {
      gdk_draw_string(pixmap,
                      font,
                      penWhite,
                      306,
                      296,
                      "Trh") ;

      sprintf(temp_string2,
              "%4.1f",
              convert_temp(ws.Trh[rh], setup_f)) ;
      gdk_draw_string(pixmap,
                      font,
                      penWhite,
                      /*356*/ 426-gdk_string_width(font,temp_string2),
                      296,
                      temp_string2) ;
      gdk_draw_string(pixmap,
                      font,
                      penWhite,
                      428,
                      296,
                      (setup_f) ? "\xb0\x46" : "\xb0\x43") ;
    }
  }
  last_rh = rh ;

  /* Fill in barometer values */
  bp = weather_primary_barom(NULL) ;

  if (bp >= 0)
  {
    char bp_val[10] ;

    /* We have a barometer */
    assert(bp<MAXBAROM);
    if ((ws.barom[bp] < 0.0) || (ws.barom[bp] > 2000.0))
    {
      werr(0, "BP%d = %f, wildly outside expected range", bp, ws.barom[bp]) ;
    }
    else
    {
      /* BP */
      char *bp_form[] = {"%6.1f", "%6.2f", "%6.2f", "%6.4f"} ;
      
      gdk_draw_string(pixmap,
                      font,
                      penWhite,
                      306,
                      330,
                      _("BP")) ;

      sprintf(bp_val, bp_form[setup_unit_bp],
              /*(setup_hg) ? "%6.2f" : "%6.1f",*/
              convert_barom(ws.barom[bp], setup_unit_bp)) ;
      gdk_draw_string(pixmap,
                      font,
                      penWhite,
                      426-gdk_string_width(font, bp_val),
                      330,
                      bp_val) ;

      gdk_draw_string(pixmap,
                      font,
                      penWhite,
                      428,
                      330,
                      convert_unit_short_name(convert_unit_millibar + setup_unit_bp)) ;
                      //(setup_hg) ? "\"Hg" : "mB") ;
    }
  }
  last_bp = bp ;

  if (devices_have_anem())
  {
	sprintf(wind_speed, "%3.0f %s", convert_speed(ws.anem_speed, setup_unit_speed),
		convert_unit_short_name(convert_unit_kph + setup_unit_speed)) ;
	gdk_draw_string(pixmap, font, penWhite,
		308+(132-gdk_string_width(font,wind_speed))/2, 30, wind_speed) ;
  }

  mainwin_draw_compass_rose(new_data, ws.vane_bearing, comp) ;

  if (devices_have_rain())
  {
    sprintf(rain, weather_rain_since(),
      convert_mm_string(ws.rain, setup_mm, 0)) ;
    sprintf(rainrate, _("(%s/hr)"),
      convert_mm_string(ws.rain_rate, setup_mm, CONVERT_NO_COMMA)) ;
  } else {
    *rain = *rainrate = '\0' ;
  }
  gdk_draw_string(pixmap, font, penBlue,
    450-gdk_string_width(font,rain), 364 /*380*/, rain) ;
  gdk_draw_string(pixmap, font, penRed,
    450-gdk_string_width(font,rainrate), 386, rainrate) ;

  /* Will copy entire pixmap onto the window */

  update_rect.x = 0 ;
  update_rect.y = 0 ;
  update_rect.width = drawing_area->allocation.width ;
  update_rect.height = drawing_area->allocation.height ;

  gtk_widget_draw(drawing_area, &update_rect) ;
}

void mainwin_check_animation(unsigned long int time_now)
{
  /* Animate the anemometer cups */

  static float anem_phase = 0.0F ;
  static int last_top = 0 ;
  int anem_pos = 0 ;

  if (!setup_anim) return ;

  /*g_warning("mainwin_check_animation(%d)", time_now);*/
  anem_phase += ws.anem_speed * ANEM_RATE ;
  if (anem_phase >= 360.0F) anem_phase -= 360.0F ;
  anem_pos = (int) floor((double) anem_phase / 120.0) ;
  anim_top_frame = anem_pos%3 ;

  if (last_top != anim_top_frame) {
    last_top = anim_top_frame ;
    mainwin_update(0 /* No new data */) ;
  }
}

static void try_default_fonts(void)
{
  char *try_fonts[]= {
    "-*-helvetica-medium-r-normal--*-240-*-*-*-*-iso8859-1",
    "-*-helvetica-medium-*-normal--*-240-*-*-*-*-iso8859-1",
    "-*-helvetica-*-*-normal--*-240-*-*-*-*-iso8859-1",
    "-*-*-*-*-normal--*-240-*-*-*-*-iso8859-1",
    "-*-*-*-*-*--*-240-*-*-*-*-iso8859-1",
    "-*-*-*-*-*--*-*-*-*-*-*-iso8859-1",
    ""
  } ;

  int i = 0 ;

  do {
    if (try_fonts[i][0] == '\0') break ;
    /* Try to load a suitable font */
    font = gdk_font_load(try_fonts[i++]) ;
  } while(!font) ;

}

void mainwin_init(void *id)
{
  if (strlen(setup_font) > 0) font = gdk_font_load(setup_font) ;
  if (!font) try_default_fonts() ;

  if (!font) g_error("gdk_font_load failed") ;

  /*mainwin_load_graphics(graphics_top, pixbuf_top, 3) ;
  mainwin_load_graphics(graphics_body, pixbuf_body, 1) ;
  mainwin_load_graphics(graphics_bottom, pixbuf_bottom, 8) ;*/
  /*ensure_graphics() ;*/
  configure_event(GTK_WIDGET(id), NULL) ;

  mainwin_init_colours() ; /* Set up the colours */

  mainwin_update(0 /* No data yet */) ;
}
