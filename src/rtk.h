/*
 *  STK2 : stage's internal graphics toolkit based on RTK2
 *
 *  Copyright (C) 2001-2005 Andrew Howard ahoward@usc.edu, Richard
 *  Vaughan vaughan@sfu.ca
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Desc: Combined Stk functions
 * Author: Andrew Howard, Richard Vaughan

 * CVS: $Id: rtk.h,v 1.7 2005-03-11 20:50:44 rtv Exp $
 */

#ifndef STK_H
#define STK_H

#include <stdio.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STK_CANVAS_LAYERS 100

// Movement mask for canvases
#define STK_MOVE_PAN   (1 << 0)
#define STK_MOVE_ZOOM  (1 << 1)

// Movement masks for figures
#define STK_MOVE_TRANS (1 << 0)
#define STK_MOVE_ROT   (1 << 1)
#define STK_MOVE_SCALE (1 << 2)

// Event codes for figures
#define STK_EVENT_PRESS     1
#define STK_EVENT_MOTION    2
#define STK_EVENT_RELEASE   3
#define STK_EVENT_MOUSE_OVER 4
#define STK_EVENT_MOUSE_NOT_OVER 5

// Export image formats
#define STK_IMAGE_FORMAT_JPEG 0
#define STK_IMAGE_FORMAT_PPM  1  
#define STK_IMAGE_FORMAT_PNG  2  
#define STK_IMAGE_FORMAT_PNM  3 

// Color space conversion macros
#define STK_RGB16(r, g, b) (((b) >> 3) | (((g) & 0xFC) << 3) | (((r) & 0xF8) << 8))
#define STK_R_RGB16(x) (((x) >> 8) & 0xF8)
#define STK_G_RGB16(x) (((x) >> 3) & 0xFC)
#define STK_B_RGB16(x) (((x) << 3) & 0xF8)

  
// Useful forward declarations
struct _stk_canvas_t;
struct _stk_table_t;
struct _stk_fig_t;
struct _stk_stroke_t;
struct _stk_region_t;
struct _stk_menuitem_t;

struct _stk_flasher_t; // rtv experimental
  
struct AVCodec;
struct AVCodecContext;


/***************************************************************************
 * Library functions
 ***************************************************************************/

// Initialise the library.
// Pass in the program arguments through argc, argv
int stk_initxx(int *argc, char ***argv);

  
/***************************************************************************
 * Application functions
 ***************************************************************************/

// Structure describing and application
typedef struct _stk_app_t
{
  int must_quit;
  int has_quit;

  // Linked list of canvases
  struct _stk_canvas_t *canvas;

  // Linked list of tables
  struct _stk_table_t *table;
  
} stk_app_t;
  
// Create the application
stk_app_t *stk_app_create(void);

// Destroy the application
void stk_app_destroy(stk_app_t *app);
  
// Main loop for the app (blocking).  Will return only when the gui is closed by
// the user.
int stk_app_main(stk_app_t *app);

// Do the initial main loop stuff
void stk_app_main_init(stk_app_t *app);

// Do the final main loop stuff
void stk_app_main_term(stk_app_t *app);

// Process pending events (non-blocking). Returns non-zero if the app
// should quit.
int stk_app_main_loop(stk_app_t *app);


/***************************************************************************
 * Canvas functions
 ***************************************************************************/
  
// Structure describing a canvas
typedef struct _stk_canvas_t
{
  // Linked list of canvases
  struct _stk_canvas_t *next, *prev;

  // Link to our parent app
  struct _stk_app_t *app;

  // Gtk stuff
  GtkWidget *frame;
  GtkWidget *layout;
  GtkWidget *canvas;
  // GDK stuff
  GdkPixmap *bg_pixmap, *fg_pixmap;
  GdkGC *gc;
  GdkColormap *colormap;
  GdkFont *font;

  // Default font name
  char *fontname;

  // Canvas background color
  GdkColor bgcolor;
  
  // Default line width
  int linewidth;

  // The menu bar widget
  GtkWidget *menu_bar;
  
  // The status bar widget
  GtkStatusbar *status_bar;

  // File in which to render xfig figures.
  FILE *file;

  // Physical size of window
  int sizex, sizey;
    
  // Coordinate transform
  // Logical coords of middle of canvas
  // and logical/device scale (eg m/pixel)
  double ox, oy;
  double sx, sy;

  // Flag set if canvas has been destroyed
  int destroyed;
  
  // Flags controlling background re-rendering
  int bg_dirty;

  // Flags controlling foreground re-rendering
  int fg_dirty;
  struct _stk_region_t *fg_dirty_region;

  // Non-zero if there are deferred calculations to be done.
  int calc_deferred;

  // Movement mask for the canvas
  int movemask;

  // Movie capture stuff
  FILE *movie_file;
  double movie_fps, movie_speed;
  int movie_frame;
  double movie_time, mpeg_time;
  unsigned char movie_lut[0x10000][3];
  struct AVCodec *movie_codec;
  struct AVCodecContext *movie_context;

  // Head of linked-list of top-level (parentless) figures.
  struct _stk_fig_t *fig;

  // Head of linked-list of figures ordered by layer.
  struct _stk_fig_t *layer_fig;

  // Head of linked-list of moveble figures.
  struct _stk_fig_t *moveable_fig;
  
  // Mouse stuff
  int mouse_mode;
  double mouse_start_x, mouse_start_y, mouse_start_a;
  struct _stk_fig_t *zoom_fig;
  struct _stk_fig_t *mouse_over_fig;
  struct _stk_fig_t *mouse_selected_fig;
  
  // linked list of figs that are shown for a short period, then
  // hidden or destroyed - rtv experimental
  struct _stk_flasher_t *flashers;
  
  // toggle these bytes to individually show and hide layers - rtv
  char layer_show[STK_CANVAS_LAYERS];

  // arbitrary user data
  void *userdata;

} stk_canvas_t;


// Create a canvas on which to draw stuff.
stk_canvas_t *stk_canvas_create(stk_app_t *app);

// Destroy the canvas
void stk_canvas_destroy(stk_canvas_t *canvas);

// See if the canvas has been closed
int stk_canvas_isclosed(stk_canvas_t *canvas);

// Set the canvas title
void stk_canvas_title(stk_canvas_t *canvas, const char *title);

// Set the size of a canvas
// (sizex, sizey) is the width and height of the canvas, in pixels.
void stk_canvas_size(stk_canvas_t *canvas, int sizex, int sizey);

// Get the canvas size
// (sizex, sizey) is the width and height of the canvas, in pixels.
void stk_canvas_get_size(stk_canvas_t *canvas, int *sizex, int *sizey);

// Set the origin of a canvas
// (ox, oy) specifies the logical point that maps to the center of the
// canvas.
void stk_canvas_origin(stk_canvas_t *canvas, double ox, double oy);

// Get the origin of a canvas
// (ox, oy) specifies the logical point that maps to the center of the
// canvas.
void stk_canvas_get_origin(stk_canvas_t *canvas, double *ox, double *oy);

// Scale a canvas
// Sets the pixel width and height in logical units
void stk_canvas_scale(stk_canvas_t *canvas, double sx, double sy);

// Get the scale of the canvas
// (sx, sy) are the pixel with and height in logical units
void stk_canvas_get_scale(stk_canvas_t *canvas, double *sx, double *sy);

// Set the movement mask
// Set the mask to a bitwise combination of STK_MOVE_TRANS, STK_MOVE_SCALE.
// to enable user manipulation of the canvas.
void stk_canvas_movemask(stk_canvas_t *canvas, int mask);

// Set the default font for text strokes
void stk_canvas_font(stk_canvas_t *canvas, const char *fontname);

// Set the canvas backround color
void stk_canvas_bgcolor(stk_canvas_t *canvas, double r, double g, double b);

// Set the default line width.
void stk_canvas_linewidth(stk_canvas_t *canvas, int width);

// Re-render the canvas
void stk_canvas_render(stk_canvas_t *canvas);

// Export an image.
// [filename] is the name of the file to save.
// [format] is the image file format (STK_IMAGE_FORMAT_JPEG, STK_IMAGE_FORMAT_PPM).
void stk_canvas_export_image(stk_canvas_t *canvas, const char *filename, int format);

/*
// Export canvas to xfig file
int stk_canvas_export_xfig(stk_canvas_t *canvas, char *filename);
*/

// Start movie capture.
// [filename] is the name of the file to save.
// [fps] is the rate at which the caller will write frames (e.g. 5fps, 10fps).
// [speed] is the ultimate playback speed of the movie (e.g. 1x, 2x).
int stk_canvas_movie_start(stk_canvas_t *canvas,
                           const char *filename, double fps, double speed);

// Start movie capture.
void stk_canvas_movie_frame(stk_canvas_t *canvas);

// Stop movie capture.
void stk_canvas_movie_stop(stk_canvas_t *canvas);

// rtv experimental
// Maintain a list of figures that are shown for a set time, then hidden.
typedef struct _stk_flasher_t
{
  struct _stk_fig_t* fig; // a figure to be shown for a set period
  int duration; // decremented on each call to stk_canvas_flash_update()
  int kill; // if zero, the fig is hidden on timeout, if non-zero, the
  // figure is destroyed instead

  // list hooks
  struct _stk_flasher_t* next, *prev;
} stk_flasher_t;

// Add this figure to the list of flashers. It will be shown until
// stk_canvas_flash_update() is called [duration] times, then it will
// be destroyed if [kill] is non-zero, or hidden, if [kill] is zero
void stk_canvas_flash( stk_canvas_t* canvas, 
		       struct _stk_fig_t* fig, int duration, int kill );

// Decrement the flasher's counters. Those that time out get hidden
// and removed from the flasher list
void stk_canvas_flash_update( stk_canvas_t* canvas );

// show and hide individual layers
void stk_canvas_layer_show( stk_canvas_t* canvas, int layer, char show );

// end rtv experimental

/***************************************************************************
 * Figure functions
 ***************************************************************************/
  
// Structure describing a color
typedef GdkColor stk_color_t;

// Callback function signatures
typedef void (*stk_mouse_fn_t) (struct _stk_fig_t *fig, int event, int mode);


// Structure describing a figure
typedef struct _stk_fig_t
{
  // Pointer to parent canvas
  struct _stk_canvas_t *canvas;

  // Pointer to our parent figure
  struct _stk_fig_t *parent;

  // Head of a linked-list of children.
  struct _stk_fig_t *child;
  
  // Linked-list of siblings
  struct _stk_fig_t *sibling_next, *sibling_prev;

  // Linked-list of figures ordered by layer  
  struct _stk_fig_t *layer_next, *layer_prev;

  // Linked-list of figures that are moveable.
  struct _stk_fig_t *moveable_next, *moveable_prev;

  // Arbitrary user data
  void *userdata;
  
  // Layer this fig belongs to
  int layer;

  // Flag set to true if figure should be displayed
  int show;

  // Movement mask (a bit vector)
  int movemask;
  
  // Origin, scale of figure
  // relative to parent.
  double ox, oy, oa;
  double cos, sin;
  double sx, sy;

  // Origin, scale of figure
  // in global cs.
  double dox, doy, doa;
  double dcos, dsin;
  double dsx, dsy;

  // Bounding box for the figure in local cs; contains all of the
  // strokes for this figure.
  double min_x, min_y, max_x, max_y;

  // Bounding region for the figure in device cs; contains all the
  // strokes for this figure.
  struct _stk_region_t *region;
  
  // List of strokes
  int stroke_size;
  int stroke_count;
  struct _stk_stroke_t **strokes;

  // Drawing context information.  Just a list of default args for
  // drawing primitives.
  stk_color_t dc_color;
  int dc_xfig_color;
  int dc_linewidth;

  // if > 0, the visibility of this figure is toggled with this
  // interval. To stop blinking but remember the interval, make the
  // value negative. (rtv)
  int blink_interval_ms;

  // Event callback functions.
  stk_mouse_fn_t mouse_fn;

  

} stk_fig_t;


// Figure creation/destruction
//stk_fig_t *stk_fig_create(stk_canvas_t *canvas, stk_fig_t *parent, int layer);
stk_fig_t *stk_fig_create(stk_canvas_t *canvas, stk_fig_t *parent, int layer );
void stk_fig_destroy(stk_fig_t *fig);

// Recursively free a whole tree of figures (rtv)
void stk_fig_and_descendents_destroy( stk_fig_t* fig );

// create a figure and set its user data pointer (rtv)
stk_fig_t *stk_fig_create_ex(stk_canvas_t *canvas, stk_fig_t *parent, 
			     int layer, void* userdata );

// Set the mouse event callback function.
void stk_fig_add_mouse_handler(stk_fig_t *fig, stk_mouse_fn_t callback);

// Unset the mouse event callback function.
void stk_fig_remove_mouse_handler(stk_fig_t *fig, stk_mouse_fn_t callback);

// Clear all existing strokes from a figure.
void stk_fig_clear(stk_fig_t *fig);

// Show or hide the figure
void stk_fig_show(stk_fig_t *fig, int show);

// Set the movement mask
// Set the mask to a bitwise combination of STK_MOVE_TRANS, STK_MOVE_ROT, etc,
// to enable user manipulation of the figure.
void stk_fig_movemask(stk_fig_t *fig, int mask);

// See if the mouse is over this figure
int stk_fig_mouse_over(stk_fig_t *fig);

// See if the figure has been selected
int stk_fig_mouse_selected(stk_fig_t *fig);

// Set the figure origin (local coordinates).
void stk_fig_origin(stk_fig_t *fig, double ox, double oy, double oa);

// Set the figure origin (global coordinates).
void stk_fig_origin_global(stk_fig_t *fig, double ox, double oy, double oa);

// Get the current figure origin (local coordinates).
void stk_fig_get_origin(stk_fig_t *fig, double *ox, double *oy, double *oa);

// Set the figure scale
void stk_fig_scale(stk_fig_t *fig, double scale);

// Set the color for strokes.  Color is specified as an (r, g, b)
// tuple, with values in range [0, 1].
void stk_fig_color(stk_fig_t *fig, double r, double g, double b);

// Set the color for strokes.  Color is specified as an RGB32 value (8
// bits per color).
void stk_fig_color_rgb32(stk_fig_t *fig, int color);

// Set the color for strokes.  Color is specified as an xfig color.
void stk_fig_color_xfig(stk_fig_t *fig, int color);

// Set the line width.
void stk_fig_linewidth(stk_fig_t *fig, int width);

// Draw a single point.
void stk_fig_point(stk_fig_t *fig, double ox, double oy);

// Draw a line between two points.
void stk_fig_line(stk_fig_t *fig, double ax, double ay, double bx, double by);

// Draw a line centered on the given point.
void stk_fig_line_ex(stk_fig_t *fig, double ox, double oy, double oa, double size);

// Draw a rectangle centered on (ox, oy) with orientation oa and size (sx, sy).
void stk_fig_rectangle(stk_fig_t *fig, double ox, double oy, double oa,
                       double sx, double sy, int filled);

// Draw an ellipse centered on (ox, oy) with orientation oa and size (sx, sy).
void stk_fig_ellipse(stk_fig_t *fig, double ox, double oy, double oa,
                     double sx, double sy, int filled);

// Draw an arc between min_th and max_th on the ellipse as above
void stk_fig_ellipse_arc( stk_fig_t *fig, double ox, double oy, double oa,
			  double sx, double sy, double min_th, double max_th);

// Create a polygon
void stk_fig_polygon(stk_fig_t *fig, double ox, double oy, double oa,
                     int point_count, double points[][2], int filled);

// Draw an arrow from point (ox, oy) with orientation oa and length len.
void stk_fig_arrow(stk_fig_t *fig, double ox, double oy, double oa,
                   double len, double head);

// Draw an arrow between points (ax, ay) and (bx, by).
void stk_fig_arrow_ex(stk_fig_t *fig, double ax, double ay,
                      double bx, double by, double head);

// create a fancy arrow that can be filled
void stk_fig_arrow_fancy(stk_fig_t *fig, double ox, double oy, double oa,
			 double len, double head, double thickness, int filled );

// Draw single or multiple lines of text.  Lines are deliminted with
// '\n'.
void stk_fig_text(stk_fig_t *fig, double ox, double oy, double oa,
                  const char *text);

// Draw a grid.  Grid is centered on (ox, oy) with size (dx, dy) with
// spacing (sp).
void stk_fig_grid(stk_fig_t *fig, double ox, double oy,
                  double dx, double dy, double sp);

// Draw an image.  bpp specifies the number of bits per pixel, and
// must be 16.
void stk_fig_image(stk_fig_t *fig, double ox, double oy, double oa,
                   double scale, int width, int height, int bpp, void *image, void *mask);

// start toggling the show flag for the figure at approximately the
// desired millisecond interval - set to < 1 to stop blinking
void stk_fig_blink( stk_fig_t* fig, int interval_ms, int flag );


/***************************************************************************
 * Primitive strokes for figures.
 ***************************************************************************/

// Signature for stroke methods.
typedef void (*stk_stroke_fn_t) (struct _stk_fig_t *fig, void *stroke);


// Structure describing a stroke (base structure for all strokes).
typedef struct _stk_stroke_t
{
  // Color
  stk_color_t color;

  // Xfig color
  int xfig_color;

  // Line width
  int linewidth;

  // Function used to free data associated with stroke
  stk_stroke_fn_t freefn;

  // Function used to compute onscreen appearance
  stk_stroke_fn_t calcfn;
  
  // Function used to render stroke
  stk_stroke_fn_t drawfn;

  // Function used to render stroke to xfig
  stk_stroke_fn_t xfigfn;
  
} stk_stroke_t;


// Struct describing a point
typedef struct
{  
  double x, y; 
} stk_point_t;


// Struct describing a point stroke
typedef struct
{
  stk_stroke_t stroke;

  // Point in logical local coordinates.
  double ox, oy;

  // Point in absolute physical coordinates.
  GdkPoint point;
  
} stk_point_stroke_t;


// Polygon/polyline stroke
typedef struct
{
  stk_stroke_t stroke;

  // Origin of figure in logical local coordinates.
  double ox, oy, oa;

  // Zero if this is a polyline, Non-zero if this is a polygon.
  int closed;
  
  // Non-zero if polygon should be filled.
  int filled;

  // A list of points in the polygon, in both local logical
  // coordinates and absolute physical coordinates.
  int point_count;
  stk_point_t *lpoints;
  GdkPoint *ppoints;
  
} stk_polygon_stroke_t;


// Struct describing text stroke
typedef struct
{
  stk_stroke_t stroke;

  // Origin of the text in logical local coordinates.
  double ox, oy, oa;

  // Origin of the text in absolute physical coordinates.
  GdkPoint point;

  // The text
  char *text;
  
} stk_text_stroke_t;


// Structure describing an image stroke
typedef struct
{
  stk_stroke_t stroke;

  // Origin of figure in local logical coordinates.
  double ox, oy, oa;

  // Rectangle containing the image (absolute physical coordinates).
  double points[4][2];

  // Image data
  double scale;
  int width, height, bpp;
  void *image, *mask;

} stk_image_stroke_t;


/***************************************************************************
 * Region manipulation (used internal for efficient redrawing).
 ***************************************************************************/

// Info about a region
typedef struct _stk_region_t
{
  // Bounding box
  GdkRectangle rect;
  
} stk_region_t;


// Create a new region.
stk_region_t *stk_region_create(void);

// Destroy a region.
void stk_region_destroy(stk_region_t *region);

// Set a region to empty.
void stk_region_set_empty(stk_region_t *region);

// Set the region to the union of the two given regions.
void stk_region_set_union(stk_region_t *regiona, stk_region_t *regionb);

// Set the region to the union of the region with a rectangle
void stk_region_set_union_rect(stk_region_t *region, int ax, int ay, int bx, int by);

// Get the bounding rectangle for the region.
void stk_region_get_brect(stk_region_t *region, GdkRectangle *rect);

// Test to see if a region is empty.
int stk_region_test_empty(stk_region_t *region);

// Test for intersection betweenr regions.
int stk_region_test_intersect(stk_region_t *regiona, stk_region_t *regionb);


/***************************************************************************
 * Menus : menus, submenus and menu items.
 ***************************************************************************/

// Callback function signatures
typedef void (*stk_menuitem_fn_t) (struct _stk_menuitem_t *menuitem);


// Info about a menu
typedef struct
{
  // Which canvas we are attached to.
  stk_canvas_t *canvas;

  // GTK widget holding the menu label widget.
  GtkWidget *item;

  // GTK menu item widget.
  GtkWidget *menu;
  
} stk_menu_t;


// Info about a menu item
typedef struct _stk_menuitem_t
{
  // Which menu we are attached to.
  stk_menu_t *menu;

  // GTK menu item widget.
  GtkWidget *item;

  // Flag set if item has been activated.
  int activated;

  // Flag set if this is a check-menu item
  int checkitem;

  // Flag set if item is checked.
  int checked;          

  // User data (mainly for callbacks)
  void *userdata;
  
  // Callback function
  stk_menuitem_fn_t callback;
  
} stk_menuitem_t;


// Create a menu
stk_menu_t *stk_menu_create(stk_canvas_t *canvas, const char *label);

// Create a sub menu
stk_menu_t *stk_menu_create_sub(stk_menu_t *menu, const char *label);

// Delete a menu
void stk_menu_destroy(stk_menu_t *menu);

// Create a new menu item.  Set check to TRUE if you want a checkbox
// with the menu item
stk_menuitem_t *stk_menuitem_create(stk_menu_t *menu,
                                    const char *label, int check);

// Set the callback for a menu item.  This function will be called
// when the user selects the menu item.
void stk_menuitem_set_callback(stk_menuitem_t *item,
                               stk_menuitem_fn_t callback);

// Delete a menu item.
void stk_menuitem_destroy(stk_menuitem_t *item);

// Test to see if the menu item has been activated.  Calling this
// function will reset the flag.
int stk_menuitem_isactivated(stk_menuitem_t *item);

// Set the check state of a menu item.
void stk_menuitem_check(stk_menuitem_t *item, int check);

// Test to see if the menu item is checked.
int stk_menuitem_ischecked(stk_menuitem_t *item);

// Enable/disable the menu item
int stk_menuitem_enable(stk_menuitem_t *item, int enable);


/***************************************************************************
 * Tables : provides a list of editable var = value pairs.
 ***************************************************************************/

// Info about a single item in a table
typedef struct _stk_tableitem_t
{
  struct _stk_tableitem_t *next, *prev;  // Linked list of items
  struct _stk_table_t *table;
  GtkWidget *label;  // Label widget
  GtkObject *adj;    // Object for storing spin box results.
  GtkWidget *spin;   // Spin box widget
  double value;      // Last recorded value of the item
} stk_tableitem_t;


// Info about a table
typedef struct _stk_table_t
{
  struct _stk_table_t *next, *prev;  // Linked list of tables
  stk_app_t *app;         // Link to our parent app
  GtkWidget *frame;       // A top-level window to put the table in.
  GtkWidget *table;       // The table layout
  int destroyed;          // Flag set to true if the GTK frame has been destroyed.
  int item_count;         // Number of items currently in the table
  int row_count;          // Number of rows currently in the table
  stk_tableitem_t *item;  // Linked list of items that belong in the table
} stk_table_t;

// Create a new table
stk_table_t *stk_table_create(stk_app_t *app, int width, int height);

// Delete the table
void stk_table_destroy(stk_table_t *table);

// Create a new item in the table
stk_tableitem_t *stk_tableitem_create_int(stk_table_t *table,
                                          const char *label, int low, int high);

// Set the value of a table item (as an integer)
void stk_tableitem_set_int(stk_tableitem_t *item, int value);

// Get the value of a table item (as an integer)
int stk_tableitem_get_int(stk_tableitem_t *item);

#ifdef __cplusplus
}
#endif

#endif

