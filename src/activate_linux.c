#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xinerama.h>

#include <cairo.h>
#include <cairo-xlib.h>

void draw(cairo_t *cr, char *title, char *subtitle, float scale) {
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.35);
    
    cairo_set_font_size(cr, 24*scale);
    cairo_move_to(cr, 20, 30*scale);
    cairo_show_text(cr, title); 
    
    cairo_set_font_size(cr, 16*scale);
    cairo_move_to(cr, 20, 55*scale);
    cairo_show_text(cr, subtitle);
}

int main(int argc, char *argv[]) {
    Display *d = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(d);
    int default_screen = XDefaultScreen(d);
    int num_entries = 0;
    XineramaScreenInfo *si = XineramaQueryScreens(d, &num_entries);

    if (si == NULL)
    {
    perror("Required X extension Xinerama is not active");
    XCloseDisplay(d);
    return 1;
    }

    for (int i = 0; i < num_entries; i++)
        printf("\nscreen number: %d\nx_org: %d\ny_org: %d\nwidth: %d\nheight: %d\n", si[i].screen_number, si[i].x_org, si[i].y_org, si[i].width, si[i].height);

    char *title, *subtitle;

    int overlay_width = 340;
    int overlay_height = 120;
    float scale = 1.0f;

    switch (argc) {
        case (1):
            #ifdef __APPLE__
            title = "Activate MacOS";
            subtitle = "Go to Settings to activate MacOS";
            #else
            title = "Activate Linux";
            subtitle = "Go to Settings to activate Linux.";
            #endif
            break;

        case (2):
            title = argv[1];
            subtitle = "";
            break;

        case (3):
            title = argv[1];
            subtitle = argv[2];
            break;

        case (4):
            title = argv[1];
            subtitle = argv[2];
            scale = atoi(argv[3]);
            break;

        default:
            printf("More than needed arguments have been passed. This program only supports at most 2 arguments.\n");
            return 1;
    } 

    XSetWindowAttributes attrs;
    attrs.override_redirect = 1;

    XVisualInfo vinfo;
    if (!XMatchVisualInfo(d, default_screen, 32, TrueColor, &vinfo)) {
        printf("No visual found supporting 32 bit color, terminating\n");
        exit(EXIT_FAILURE);
    }

    // sets 32 bit color depth
    attrs.colormap = XCreateColormap(d, root, vinfo.visual, AllocNone);
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;

    Window overlay[num_entries];
    cairo_surface_t *surface[num_entries];
    cairo_t *cairo_ctx[num_entries];

    for (int i = 0; i < num_entries; i++)
    {
    overlay[i] = XCreateWindow(
            d,                                                                     // display
            root,                                                                  // parent
            si[i].x_org + si[i].width - overlay_width * scale,               // x position
            si[i].y_org + si[i].height - overlay_height * scale,             // y position
            overlay_width * scale,                                                 // width
            overlay_height * scale,                                                // height
            0,                                                                     // border width
            vinfo.depth,                                                           // depth
            InputOutput,                                                           // class
            vinfo.visual,                                                          // visual
            CWOverrideRedirect | CWColormap | CWBackPixel | CWBorderPixel,         // value mask
            &attrs                                                                 // attributes
        );
    
        XMapWindow(d, overlay[i]);

        // allows the mouse to click through the overlay
        XRectangle rect;
        XserverRegion region = XFixesCreateRegion(d, &rect, 1);
        XFixesSetWindowShapeRegion(d, overlay[i], ShapeInput, 0, 0, region);
        XFixesDestroyRegion(d, region);

        // cairo context
        surface[i] = cairo_xlib_surface_create(d, overlay[i], vinfo.visual, overlay_width * scale, overlay_height * scale);
        cairo_ctx[i] = cairo_create(surface[i]);
        draw(cairo_ctx[i], title, subtitle, scale);
    }
    
    // wait for X events forever
    XEvent event;
    while(1) {
        XNextEvent(d, &event);
    }  

    for (int i = 0; i < num_entries; i++)
    {
        XUnmapWindow(d, overlay[i]);
        cairo_destroy(cairo_ctx[i]);
        cairo_surface_destroy(surface[i]);
    }

    XFree(si);
    XCloseDisplay(d);
    return 0;
}
