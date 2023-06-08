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
#include "defaultCube.h"

void genDefaultCubeScene(void) 
{
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glBegin(GL_QUADS);
    /* face +Z */
    glNormal3f(0, 0, 1);
    glColor3f(1, 0, 0);
    glVertex3f(-1, -1, 1);
    glVertex3f(1, -1, 1);
    glVertex3f(1, 1, 1);
    glVertex3f(-1, 1, 1);
    /* face +X */
    glNormal3f(1, 0, 0);
    glColor3f(0, 1, 0);
    glVertex3f(1, -1, 1);
    glVertex3f(1, -1, -1);
    glVertex3f(1, 1, -1);
    glVertex3f(1, 1, 1);
    /* face -Z */
    glNormal3f(0, 0, -1);
    glColor3f(0, 0, 1);
    glVertex3f(1, -1, -1);
    glVertex3f(-1, -1, -1);
    glVertex3f(-1, 1, -1);
    glVertex3f(1, 1, -1);
    /* face -X */
    glNormal3f(-1, 0, 0);
    glColor3f(1, 1, 0);
    glVertex3f(-1, -1, -1);
    glVertex3f(-1, -1, 1);
    glVertex3f(-1, 1, 1);
    glVertex3f(-1, 1, -1);
    /* face +Y */
    glNormal3f(0, 1, 0);
    glColor3f(0, 1, 1);
    glVertex3f(-1, 1, 1);
    glVertex3f(1, 1, 1);
    glVertex3f(1, 1, -1);
    glVertex3f(-1, 1, -1);
    /* face -Y */
    glNormal3f(0, -1, 0);
    glColor3f(1, 0, 1);
    glVertex3f(-1, -1, -1);
    glVertex3f(1, -1, -1);
    glVertex3f(1, -1, 1);
    glVertex3f(-1, -1, 1);
}

void redrawDefaultCubeScene(const spnav_posrot *posrot, unsigned int scene, Display *dpy, Window &win) 
{
	float xform[16];

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -20);	/* view matrix, push back to see the cube */

    /* XXX convert the accumulated position/rotation into a 4x4 view matrix */
    spnav_matrix_obj(xform, posrot);
    glMultMatrixf(xform);		/* concatenate our computed view matrix */


    glCallList(scene);
    genDefaultCubeScene();

    glXSwapBuffers(dpy, win);
}