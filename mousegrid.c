#include <cairo.h>
#include <gtk/gtk.h>

//Font height as a fraction of the height of the containing box
#define FONT_HEIGHT_FRACTION 2.0
#define MAX_FONT_HEIGHT_PX 100
#define COLOR_LINES 1.0, 1.0, 1.0, 0.8
#define COLOR_TEXT  1.0, 0.0, 0.0, 0.9
#define COLOR_BACKGROUND 0.0, 0.0, 0.0, 0.3
#define FONT "Sans Bold"
//Default size of GTK window. To detect when fullscreen window is initialised
#define DEFAULT_WIDTH 200
#define MAX_DEPTH 5

gboolean draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data);
gint key_press(GtkWidget *widget, GdkEventKey *kevent, gpointer data);

GdkScreen *screen;

typedef struct _AppState {
    short int x, y;
    short int width, height;
} AppState;

AppState *appStateStack;
int stackPointer;


int
main (int argc, char *argv[])
{
  appStateStack = malloc(MAX_DEPTH*sizeof(AppState));
  stackPointer = -1;

  //For debugging purposes, prevent messages from only printing at end of program
  setbuf(stdout, NULL);

  gdk_set_show_events(TRUE);
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

  gtk_widget_show (window);
  g_signal_connect (window, "draw", G_CALLBACK (draw_callback), NULL);
  g_signal_connect(window, "key_press_event", G_CALLBACK(key_press), NULL);

  gtk_main ();

  return 0;
}


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
    //Draw is called before fullscreen takes effect, so need to ensure that widget width is not still returning the default GTK widget width
    if (stackPointer == -1) {
        int width = gtk_widget_get_allocated_width (widget);
        if (width != DEFAULT_WIDTH) {
            //Initialise appState
            AppState a;
            a.x = a.y = 0;
            a.width = width;
            a.height = gtk_widget_get_allocated_height (widget);
            appStateStack[++stackPointer] = a;
        }
    }
    //Draw semi-transparent background on the window
    cairo_set_source_rgba(cr, COLOR_BACKGROUND);
    //Paint over existing data (instead of drawing on top of the grey background)
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);

    AppState appState = appStateStack[stackPointer];
    draw_grid(cr, appState.x, appState.y, appState.width, appState.height);

    return TRUE;
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
        if (kevent->keyval >= GDK_KEY_1 && kevent->keyval <= GDK_KEY_9) {
            if (stackPointer < MAX_DEPTH) {
                int row = (kevent->keyval-48 - 1)/3;
                int col = (kevent->keyval-48 - 1)%3;

                AppState oldState = appStateStack[stackPointer];
                AppState newState;
                newState.width = oldState.width/3.0;
                newState.height = oldState.height/3.0;
                newState.x = oldState.x + newState.width*col;
                newState.y = oldState.y + newState.height*row;
                appStateStack[++stackPointer] = newState;
                gtk_widget_queue_draw(widget);

                int x = newState.x + newState.width/2.0;
                int y = newState.y + newState.height/2.0;
                move_mouse(screen, x, y);
            }
        }
        else if (kevent->keyval == GDK_KEY_minus) {//Undo last
            if (stackPointer > 0) {
                stackPointer--;
                AppState a = appStateStack[stackPointer];
                int x = a.x + a.width/2.0;
                int y = a.y + a.height/2.0;
                move_mouse(screen, x, y);
                gtk_widget_queue_draw(widget);
            }
        }
        else if (kevent->keyval == GDK_KEY_KP_Enter) {
            gtk_widget_hide(widget);
        }

    }
    return TRUE;
}
