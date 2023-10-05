#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define _NET_WM_STATE_REMOVE 0 // remove/unset property
#define _NET_WM_STATE_ADD 1    // add/set property
#define _NET_WM_STATE_TOGGLE 2 // toggle property

typedef struct Hints
{
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long inputMode;
    unsigned long status;
} Hints;

static void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

Window initWindow(Display *display, unsigned int *width, unsigned int *height, int *screen)
{
    XVisualInfo vinfo;
    XSetWindowAttributes attr;

    *screen = DefaultScreen(display);
    *width = DisplayWidth(display, *screen);
    *height = DisplayHeight(display, *screen);
    printf("%u %u\n", *width, *height);

    // window
    XMatchVisualInfo(display, *screen, 32, TrueColor, &vinfo);
    attr.colormap = XCreateColormap(display, DefaultRootWindow(display), vinfo.visual, AllocNone);
    // attr.colormap = DefaultColormap(display, *screen);
    attr.border_pixel = 0;
    attr.background_pixel = 0;
    attr.override_redirect = 1;
    Window window = XCreateWindow(display, DefaultRootWindow(display), 0, 0, *width, *height, 10, vinfo.depth,
                                  InputOutput, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attr);

    // attributes and properties
    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    Atom window_properties = XInternAtom(display, "_MOTIF_WM_HINTS", 1);
    Hints hints;
    hints.flags = 2; // i don't actually know what the fuck is this
    hints.decorations = 0;
    XSetWMProtocols(display, window, &wm_delete_window, 1);
    XChangeProperty(display, window, window_properties, window_properties, 32, PropModeReplace, (unsigned char *)&hints, 5);
    XserverRegion region = XFixesCreateRegion(display, NULL, 0);
    XFixesSetWindowShapeRegion(display, window, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(display, region);
    XSelectInput(display, window, ExposureMask | StructureNotifyMask | SubstructureRedirectMask);
    XMapWindow(display, window);
    return window;
}

GC createGC(Display *display, Window *window, int screen)
{
    XGCValues xgc;
    GC gc = XCreateGC(display, *window, 0, &xgc);
    XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinBevel);
    XSetFillStyle(display, gc, FillSolid);
    XSetForeground(display, gc, XWhitePixel(display, screen));
    return gc;
}

void xDraw(Display *display, Window *window, GC gc, unsigned int width, unsigned int height, int diameter)
{
    XSync(display, False);

    XDrawLine(display, *window, gc, width >> 1, 0, width >> 1, height);
    XDrawLine(display, *window, gc, 0, height >> 1, width, height >> 1);
    XDrawArc(display, *window, gc, (width >> 1) - (diameter >> 1), (height >> 1) - (diameter >> 1), diameter, diameter, 0, 360 * 64);

    XSync(display, False);
    XMapWindow(display, *window);
    XFlush(display);
}

int main(int argc, char **argv)
{
    Display *display;
    Window window;
    GC gc;
    int screen;

    // init
    display = XOpenDisplay(NULL);
    if (!display)
        die("display error");

    unsigned int width = 0;
    unsigned int height = 0;

    window = initWindow(display, &width, &height, &screen);
    gc = createGC(display, &window, screen);

    XEvent ev;
    int keep_running = 1;
    while (keep_running)
    {
        XNextEvent(display, &ev);

        switch (ev.type)
        {
        case Expose:
            xDraw(display, &window, gc, width, height, 400);
            break;
        case ClientMessage:
            if (ev.xclient.message_type == XInternAtom(display, "WM_PROTOCOLS", 1) && (Atom)ev.xclient.data.l[0] == XInternAtom(display, "WM_DELETE_WINDOW", 1))
                keep_running = 0;
            break;
        default:
            break;
        }
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}