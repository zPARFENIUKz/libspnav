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
            redrawMyCubeScene(&posrot, scene, dpy, win);
            break;
        case 2:
            redrawDefaultCubeScene(&posrot, scene, dpy, win);
            break;
        case 3:
            redrawFlyScene(&posrot, scene, spy, win);
    }
}