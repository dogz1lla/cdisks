// check introcs.cs.princeton.edu/java/assignments/collisions.html
// for an idea on how to improve the simulation (better robustness 
// and efficiency) by using event-driven simulation scheme
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xdbe.h>
#include <assert.h>
#include <unistd.h>


double myRandom(double a, double b) {
    /* 
       Small utility function that returns a 
       double random value between a and b
     */
    if (b <= a) {
        printf("Error in random!\n");
        exit(1);
    }
    double r = 0;
    return (b-a)*rand()/((double)RAND_MAX) + a;
}


typedef struct {
    /*
        Struct that corresponds to the
        bounding box inside which our 
        particles live. Has width w and
        height h.
     */
    int w;
    int h;
} Box;


GC
create_gc(Display* display, Window win, int reverse_video)
{
    /*
        create graphical context for X. this code is 
        copied from slist.lilotux.net/linux/xlib/simple-window.c
     */
  GC gc;				/* handle of newly created GC.  */
  unsigned long valuemask = 0;		/* which values in 'values' to  */
					/* check when creating the GC.  */
  XGCValues values;			/* initial values for the GC.   */
  unsigned int line_width = 2;		/* line width for the GC.       */
  int line_style = LineSolid;		/* style for lines drawing and  */
  int cap_style = CapButt;		/* style of the line's edje and */
  int join_style = JoinBevel;		/*  joined lines.		*/
  int screen_num = DefaultScreen(display);

  gc = XCreateGC(display, win, valuemask, &values);
  if (gc < 0) {
    printf("error!\n");
	fprintf(stderr, "XCreateGC: \n");
  }

  /* allocate foreground and background colors for this GC. */
  if (reverse_video) {
    XSetForeground(display, gc, WhitePixel(display, screen_num));
    XSetBackground(display, gc, BlackPixel(display, screen_num));
  }
  else {
    XSetForeground(display, gc, BlackPixel(display, screen_num));
    XSetBackground(display, gc, WhitePixel(display, screen_num));
  }

  /* define the style of lines that will be drawn using this GC. */
  XSetLineAttributes(display, gc,
                     line_width, line_style, cap_style, join_style);

  /* define the fill style for the GC. to be 'solid filling'. */
  XSetFillStyle(display, gc, FillSolid);

  return gc;
}

int main(){
    Display *dpy = XOpenDisplay(getenv("DISPLAY"));
    assert(dpy);

    int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
    int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

    // create a window
    Window w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
                                   200, 100, 0, blackColor, blackColor);
    
    // we want to get MapNotify events
    XSelectInput(dpy, w, StructureNotifyMask);

    // "map" the window (i.e., make it appear on the screen)
    XMapWindow(dpy, w);

    // create a "Graphics Context"
    //GC gc = XCreateGC(dpy, w, 0, NULL);
    GC gc = create_gc(dpy, w, 1);
    GC gc_inv = create_gc(dpy, w, 0);

    // tell the GC we draw using the white color
    XSetForeground(dpy, gc, whiteColor);

    // wait for the MapNotify event
    for(;;) {
        XEvent e;
        XNextEvent(dpy, &e);
        if (e.type == MapNotify)
                break;
    }

    int i=0, j=0;
    const int n_particles = 150;
    Box *box = malloc(sizeof(Box));
    double x[n_particles];
    double y[n_particles];
    double vx[n_particles];
    double vy[n_particles];
    int width = 500;
    int height = 650;
    box->w = width;
    box->h = height;
    double R = 7.0;
    double v0 = 2000.0;
    double dt = 0.0001;
    srand(time(NULL));
    double inner_box_width = box->w - 2*R;
    double inner_box_height = box->h - 2*R;
    double s_inner = inner_box_width*inner_box_height;
    double s0 = s_inner/n_particles;
    double s0x = sqrt(s0);
    double s0y = s0x;
    double d_crit = 4*R;
    if (s0x < d_crit) {
        printf("Density too large!\n");
        free(box);
        return 1;
    }

    double dx = d_crit; 
    double x0 = dx;
    double y0 = dx;
    for (i=0; i<n_particles; i++) {
        x[i] = x0;
        y[i] = y0;
        x0 += dx;
        if (x0 > box->w - d_crit) {
            x0 = dx;
            y0 += dx;
        }
        vx[i] = myRandom(-v0, v0);
        vy[i] = myRandom(-v0, v0);
    }

    XdbeSwapAction swap_action = XdbeBackground;
    XdbeSwapInfo *swap_info = malloc(sizeof(XdbeSwapInfo));
    swap_info->swap_window = w;
    swap_info->swap_action = swap_action;
    XdbeBackBuffer bb = XdbeAllocateBackBufferName(dpy, w, swap_action);
    for (i=0; i<n_particles; i++) {
        XFillArc(dpy, w, gc, x[i]-R, y[i]-R, R*2, R*2, 0, 360*64);
    }

    while(1) {
        for (i=0; i<n_particles; i++) {
            x[i] += vx[i]*dt;
            y[i] += vy[i]*dt;
        }

        /* draw a bunch of circles at random locations */
        for (i=0; i<n_particles-1; i++) {
            double xi = x[i];
            double yi = y[i];
            for (j=i+1; j<n_particles; j++) {
                double xj = x[j];
                double yj = y[j];
                double r2 = (xi-xj)*(xi-xj)+(yi-yj)*(yi-yj);
                if (r2<=4*R*R) {
                    double vxtemp = vx[i], vytemp = vy[i];
                    vx[i] = vx[j];
                    vy[i] = vy[j];
                    vx[j] = vxtemp;
                    vy[j] = vytemp;
                }
            }
            if (xi < R || xi > box->w - R) {
                vx[i] *= -1;
            }
            if (yi < R || yi > box->h - R) {
                vy[i] *= -1;
            }
        }

        for (i=0; i<n_particles; i++) {
            XFillArc(dpy, bb, gc, x[i]-R, y[i]-R, R*2, R*2, 0, 360*64);
        }

        XdbeSwapBuffers(dpy, swap_info, 1);
        // send the "DrawLine" request to the server
        XFlush(dpy);

        //usleep((int)dt*1.0e6);
        usleep(1000);
    }
    free(box);
    return 0;
}
