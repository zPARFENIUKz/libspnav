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
#include "fly.h"

void genFlyScene(void) 
{
	float x, y, h;


    glEnable(GL_TEXTURE_2D);

    /* grid */
    glBindTexture(GL_TEXTURE_2D, grid_tex);

    glBegin(GL_QUADS);
    glColor3f(1, 1, 1);
    glTexCoord2f(0, 0);
    glVertex3f(-GRID_SZ, 0, GRID_SZ);
    glTexCoord2f(GRID_REP, 0);
    glVertex3f(GRID_SZ, 0, GRID_SZ);
    glTexCoord2f(GRID_REP, GRID_REP);
    glVertex3f(GRID_SZ, 0, -GRID_SZ);
    glTexCoord2f(0, GRID_REP);
    glVertex3f(-GRID_SZ, 0, -GRID_SZ);
    glEnd();

    /* buildings */
    glBindTexture(GL_TEXTURE_2D, box_tex);
    for(i=0; i<8; i++) 
    {
        for(j=0; j<8; j++) 
        {
            x = (j - 4.0f + 0.5f * (float)rand() / RAND_MAX) * 20.0f;
            y = (i - 4.0f + 0.5f * (float)rand() / RAND_MAX) * 20.0f;
            h = (3.0f + (float)rand() / RAND_MAX) * 6.0f;

            glPushMatrix();
            glTranslatef(x, h/2, y);
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glScalef(3, h/4, 1);

            draw_box(6, h, 6);

            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        }
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    /* skydome */
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.07, 0.1, 0.4);
    glVertex3f(0, GRID_SZ/5, 0);
    glColor3f(0.5, 0.2, 0.05);
    glVertex3f(-GRID_SZ, 0, -GRID_SZ);
    glVertex3f(GRID_SZ, 0, -GRID_SZ);
    glVertex3f(GRID_SZ, 0, GRID_SZ);
    glVertex3f(-GRID_SZ, 0, GRID_SZ);
    glVertex3f(-GRID_SZ, 0, -GRID_SZ);
    glEnd();
}
void redrawFlyScene(const posrot *posrot, unsigned int scene, Display *dpy, Window &win) 
{
    float xform[16];

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* XXX convert the accumulated position/rotation into a 4x4 view matrix */
    spnav_matrix_view(xform, &posrot);
    glMultMatrixf(xform);		/* concatenate our computed view matrix */
    glTranslatef(0, -5, 0);		/* move the default view a bit higher above the ground */

    glCallList(scene);

    glXSwapBuffers(dpy, win);
}