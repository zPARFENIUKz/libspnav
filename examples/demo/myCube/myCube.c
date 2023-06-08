#include "myCube.h"
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
#include <X11/Xatom.h>


void genMyCubeScene() 
{
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    int i = 0, j = 0, k = 0;
    glBegin(GL_QUADS);
    for (i = 0; i < 9; ++i) 
    {
        for (j = 0; j < 9; ++j) 
        {
            for (k = 0; k < 9; ++k) 
            {
                /*cube*/
                /* face +Z */
                glNormal3f(0, 0, 1);
                glColor3f(1, 1, 1);
                glVertex3f(-5 + i, -5 + j, -3 + k);
                glVertex3f(-3 + i, -5 + j, -3 + k);
                glVertex3f(-3 + i, -3 + j, -3 + k);
                glVertex3f(-5 + i, -3 + j, -3 + k);
                /* face +X */
                glNormal3f(1, 0, 0);
                glColor3f(1, 1, 1);
                glVertex3f(-3 + i, -5 + j, -3 + k);
                glVertex3f(-3 + i, -5 + j, -5 + k);
                glVertex3f(-3 + i, -3 + j, -5 + k);
                glVertex3f(-3 + i, -3 + j, -3 + k);
                /* face -Z */
                glNormal3f(0, 0, -1);
                glColor3f(1, 1, 1);
                glVertex3f(-3 + i, -5 + j, -5 + k);
                glVertex3f(-5 + i, -5 + j, -5 + k);
                glVertex3f(-5 + i, -3 + j, -5 + k);
                glVertex3f(-3 + i, -3 + j, -5 + k);
                /* face -X */
                glNormal3f(-1, 0, 0);
                glColor3f(1, 1, 1);
                glVertex3f(-5 + i, -5 + j, -5 + k);
                glVertex3f(-5 + i, -5 + j, -3 + k);
                glVertex3f(-5 + i, -3 + j, -3 + k);
                glVertex3f(-5 + i, -3 + j, -5 + k);
                /* face +Y */
                glNormal3f(0, 1, 0);
                glColor3f(1, 1, 1);
                glVertex3f(-5 + i, -3 + j, -3 + k);
                glVertex3f(-3 + i, -3 + j, -3 + k);
                glVertex3f(-3 + i, -3 + j, -5 + k);
                glVertex3f(-5 + i, -3 + j, -5 + k);
                /* face -Y */
                glNormal3f(0, -1, 0);
                glColor3f(1, 1, 1);
                glVertex3f(-5 + i, -5 + j, -5 + k);
                glVertex3f(-3 + i, -5 + j, -5 + k);
                glVertex3f(-3 + i, -5 + j, -3 + k);
                glVertex3f(-5 + i, -5 + j, -3 + k);
             }
         }
     }
}

void redrawMyCubeScene(const spnav_posrot *posrot, unsigned int scene, Display *dpy, Window &win) 
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
    genMyCubeScene();

    glXSwapBuffers(dpy, win);
}