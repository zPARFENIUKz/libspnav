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
#include "xwin.h"
#include <stdbool.h>
#include "myCube/myCube.h"
#include "defaultCube/defaultCube.h"
#include "fly/fly.h"

#define GRID_REP	180
#define GRID_SZ		200

const char* connectDeviceMessage = "Connect your device to continue...";
const char* failedConnectionWithDaemonMessage = "Connection with the daemon was failed";

struct spnav_posrot posrot;
unsigned int grid_tex, box_tex;
unsigned int scene;
bool isPrintedAboutDevice = false;
bool isPrintedAboutConnectDevice = false;
int xsock, ssock, maxfd;
char buf[256];
spnav_event sev;
int demoNumber = 1;

void gen_textures(void);
void gen_scene(void);
void redraw(void);
void draw_scene(void);
void handle_spnav_event(spnav_event *ev);
int handle_xevent(XEvent *xev);
void draw_box(float xsz, float ysz, float zsz);

void openConnectionWithDaemon();
bool isDeviceConnected();
void tryToPrintDeviceInfo();
bool isAnyButtonWasPressed();
void openDemoWindow();
void tryToPrintConnectDeviceMessage(const char* msg);
void runDemoInWindow();

/*demos redraw functions declarations*/
void redrawMyCubeScene(void);
void redrawDefaultCubeScene(void);
void redrawFlyScene(void);

int main(void) 
{
	for (;;) 
	{
		openConnectionWithDaemon();
		if (isDeviceConnected()) {
			if (!isPrintedAboutDevice)
			{
				tryToPrintDeviceInfo();
				isPrintedAboutDevice = true;
				isPrintedAboutConnectDevice = false;
			}
			if (isAnyDeviceButtonWasPressed()) 
			{
				openDemoWindow();
			} else 
			{
				continue;
			}
		} else 
		{
			if (!isPrintedAboutConnectDevice) 
			{
				tryToPrintConnectDeviceMessage(connectDeviceMessage);
				isPrintedAboutDevice = false;
				isPrintedAboutConnectDevice = true;
			}
			continue;
		}
		runDemoInWindow();
	}
	return 0;
}

void openConnectionWithDaemon() 
{
	if (spnav_open() == -1) 
	{
		printf(failedConnectionWithDaemonMessage);
	}
}

bool isDeviceConnected() 
{
	return spnav_dev_name(buf, sizeof buf) != -1;
}

void tryToPrintDeviceInfo() 
{
	spnav_dev_name(buf, sizeof buf);
	system("clear");
	printf("Device: %s\n", buf);
	printf("Press any device button to continue...\n");
}

bool isAnyDeviceButtonWasPressed() 
{
	if(spnav_poll_event(&sev))
        {
            if (sev.type == SPNAV_EVENT_BUTTON)
            {
                return true;
            }
        }
       return false;
}

void openDemoWindow() 
{
	if(!(dpy = XOpenDisplay(0))) 
	{
    	printf("failed to connect to the X server");
    	return false;
    }

    if(create_xwin("libspnav fly", 1080, 1080) == -1) {
   	    printf("create_xwin failed");
    	return false;
    }

    /* XXX: initialize the position vector & orientation quaternion */
   	spnav_posrot_init(&posrot);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, GRID_SZ / 4);
    glFogf(GL_FOG_END, GRID_SZ);

    gen_textures();
    gen_scene();

    /* XXX: grab the Xlib socket and the libspnav socket. we'll need them in the
    * select loop to wait for input from either source.
     */
   	xsock = ConnectionNumber(dpy);		/* Xlib socket */
    ssock = spnav_fd();					/* libspnav socket */
    maxfd = xsock > ssock ? xsock : ssock;
    return true;
}

void tryToPrintConnectDeviceMessage(const char* msg) 
{
	printf(msg);
}

void runDemoInWindow() 
{
	for(;;) {
    		fd_set rdset;

    		/* XXX: add both sockets to the file descriptor set, to monitor both */
    		FD_ZERO(&rdset);
    		FD_SET(xsock, &rdset);
    		FD_SET(ssock, &rdset);

    		while(select(maxfd + 1, &rdset, 0, 0, 0) == -1 && errno == EINTR);

    		/* XXX: handle any pending X events */
    		if(FD_ISSET(xsock, &rdset)) {
    			while(XPending(dpy)) {
    				XEvent xev;
    				XNextEvent(dpy, &xev);

    				if(handle_xevent(&xev) != 0) {
    					goto end;
    				}
    				if (!tryToPrintDevice(buf))
    				{
    				    goto end;
    				}
    			}
    		}

    		/* XXX: handle any pending spacenav events */
    		if(FD_ISSET(ssock, &rdset)) {
    			while(spnav_poll_event(&sev)) {
    				handle_spnav_event(&sev);
    			}
    		}

    		if(redisplay_pending) {
    			redisplay_pending = 0;
    			redraw();
    		}
    	}

    end:
    	glDeleteTextures(1, &grid_tex);
    	destroy_xwin();
    	spnav_close();  
}

void redraw(void) 
{
    switch demoNumber 
    {
        case 1:
            redrawMyCubeScene();
            break;
        case 2:
            redrawDefaultCubeScene();
            break;
        case 3:
            redrawFlyScene();
    }
}

/*myCube demo functions*/
void genMyCubeScene(void) 
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

void redrawMyCubeScene(void) 
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

/*defaultCube demo functions*/
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

void redrawDefaultCubeScene(void) 
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


/*fly demo functions*/
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
void redrawFlyScene(void) 
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

void gen_scene(void) 
{
    float x, y, h;

	srand(0);

	scene = glGenLists(1);
	glNewList(scene, GL_COMPILE);

	glEnable(GL_TEXTURE_2D);
	/*glEnable(GL_FOG)*/;


	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glTranslatef(0, 0, 0);	/* view matrix, push back to see the cube */
    switch demoNumber 
    {
        case 1:
            genMyCubeScene();
            break;
        case 2:
            genDefaultCubeScene();
            break;
        case 3:
            genFlyScene();
            break;
    }
    if (demoNumber != 3) glEnd();
    glEndList();
}