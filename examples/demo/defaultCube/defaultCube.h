#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <spnav.h>
#include "../xwin.h"
#include <stdbool.h>

void genDefaultCubeScene(void);
void redrawDefaultCubeScene(const posrot *posrot, unsigned int scene, Display *dpy, Window &win);