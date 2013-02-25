#include <cairo.h>
#include <gtk/gtk.h>

//Font height as a fraction of the height of the containing box
#define FONT_HEIGHT_FRACTION 2.0
#define MAX_FONT_HEIGHT_PX 100
#define COLOR_LINES 1.0, 1.0, 1.0, 0.8
#define COLOR_TEXT  1.0, 0.0, 0.0, 0.6
#define COLOR_BACKGROUND 0.0, 0.0, 0.0, 0.3
#define FONT "Sans Bold"

GdkScreen *screen;

void draw_text(cairo_t *cr, const char *text, const int x, const int y, const int box_height) {
    PangoLayout *layout;
    layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text, -1);

    PangoFontDescription *desc;
    desc = pango_font_description_from_string(FONT);

    //Set font to fraction to font height fraction, but no more than the maximum font height
    int size_px;
    if (box_height/FONT_HEIGHT_FRACTION >= MAX_FONT_HEIGHT_PX) {
        size_px = MAX_FONT_HEIGHT_PX;
    } else {
        size_px = box_height / FONT_HEIGHT_FRACTION;
    }
    pango_font_description_set_absolute_size (desc, PANGO_SCALE * size_px);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    //Center text
    int pxw, pxh;
    pango_layout_get_pixel_size(layout, &pxw, &pxh);
    cairo_move_to (cr, x - pxw/2, y - pxh/2);

    pango_cairo_update_layout(cr, layout);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
}

void
draw_grid(cairo_t *cr, const int x, const int y, const int width, const int height) {
    const int LINES = 3;
    const float LINESF = (float) LINES;

    cairo_set_source_rgba(cr, COLOR_LINES);
    for(int i=0; i<=LINES; i++) {
        cairo_move_to(cr, x+width*(i/LINESF), y);  
        cairo_line_to(cr, x+width*(i/LINESF), y+height);
    }
    for(int i=0; i<=LINES; i++) {
        cairo_move_to(cr, x, y+height*(i/LINESF));  
        cairo_line_to(cr, x+width, y+height*(i/LINESF));
        cairo_stroke(cr);
    }

    cairo_set_source_rgba(cr, COLOR_TEXT);
    for(int i=0; i<LINES; i++) {
        float text_y = y+height*(i*2+1)/(LINESF*2);
        for (int j=0; j<LINES; j++) {
            float text_x = x+width*(j*2+1)/(LINESF*2);
            char txt[1];
            sprintf(txt, "%d", i*LINES + j + 1);
            draw_text(cr, txt, text_x, text_y, height/LINES);
        }
    }
}

/**
 * Called on window draw event
 */
gboolean
draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
    //Draw semi-transparent background on the window
    cairo_set_source_rgba(cr, COLOR_BACKGROUND);
    //Paint over existing data (instead of drawing on top of the grey background)
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE); 
    cairo_paint(cr);

    guint width, height;
    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);

    //draw_grid(cr, 200, 200, width/3, height/3);
    draw_grid(cr, 0, 0, width, height);

    return FALSE;
}

void
move_mouse (GdkScreen *screen, int x, int y) {
    GdkDisplay *display;
    display = gdk_display_get_default();
    GdkDeviceManager *device_manager;
    device_manager = gdk_display_get_device_manager(display);
    GdkDevice *pointer;
    pointer = gdk_device_manager_get_client_pointer(device_manager);
    gdk_device_warp(pointer, screen, x, y);
}

gint key_press(GtkWidget *widget, GdkEventKey *kevent, gpointer data)  {
if(kevent->type == GDK_KEY_PRESS)  {
int number = kevent->keyval - 48;
if (number >= 1 && number <= 9) {
    guint width, height;
    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);
    int row = (number - 1)/3;
    int col = (number - 1)%3;
    int h = (height*((row*2+1)/6.0));
    int w = (width*((col*2+1)/6.0));
    printf("%d", h);
    move_mouse(screen, w, h);
}
    }

    return TRUE;
}

int
main (int argc, char *argv[])
{
  //For debugging purposes, prevent messages from only printing at end of program
  setbuf(stdout, NULL);

  GtkWidget *window;
  GdkVisual *visual;
  gtk_init (&argc, &argv);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
  gtk_window_fullscreen(GTK_WINDOW(window));
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  //Enable transparency
  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
  screen = gtk_window_get_screen (GTK_WINDOW (window));
  visual = gdk_screen_get_rgba_visual (screen);
  if (visual == NULL) {
    fprintf(stderr, "ERROR: Your window manager doesn't support transparency.");
    return 1;
    visual = gdk_screen_get_system_visual (screen);
  }
  gtk_widget_set_visual(GTK_WIDGET (window), visual);

  move_mouse(screen, 100,10);

  GType	t = G_TYPE_FROM_INSTANCE(window);
  guint *blah, n;
  blah = g_signal_list_ids(t, &n);
  for (int i=0; i<sizeof(blah)/sizeof(blah[0]); i++) {
    printf("%d ", blah[i]);
  }
  g_signal_connect (window, "draw", G_CALLBACK (draw_callback), NULL);
  g_signal_connect(window, "key_press_event", G_CALLBACK(key_press), NULL);
  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
