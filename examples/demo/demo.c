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
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define GRID_REP	180
#define GRID_SZ		200

const char *spaceballInfoDirectoryFilePath = "spaceballs/info/";
const char *spaceballSpnavrcDirectoryFilePath = "spaceballs/spnavrcs/";
char prevDevice[128] = "\0";

void gen_textures(void);
void gen_scene(void);
void redraw(void);
void draw_scene(void);
void handle_spnav_event(spnav_event *ev);
int handle_xevent(XEvent *xev);
void draw_box(float xsz, float ysz, float zsz);

void genMyCubeScene(void);
void genDefaultCubeScene(void);
void genFlyScene(void);

struct spnav_posrot posrot;

unsigned int grid_tex, box_tex;
bool isDemoStarted = false;
unsigned int scene;
bool isPrintedAboutDevice = false;
bool isPrintedAboutConnectDevice = false;
int xsock, ssock, maxfd;
char buf[256] = "\0";
spnav_event sev;
int demoNumber = 1;
pid_t spacenavdPid = 0;
const char *spacenavdDefaultStartScript = "./spacenavd/spacenavd -v";
const char *spacenavdWithSpnavrcStartScript = "./spacenavd/spacenavd -v -c ";


void startDefaultSpacenavd(void) 
{
    system(spacenavdDefaultStartScript);
}

void stopSpacenavd(void) 
{
    kill(spacenavdPid, SIGKILL);
}

bool isThereSpnavrcForDevice(const char *deviceName) {
    char spnavrcFilePath[128] = "\0";
    
    strcat(spnavrcFilePath, spaceballSpnavrcDirectoryFilePath);
    strcat(spnavrcFilePath, deviceName);
    strcat(spnavrcFilePath, "/spnavrc");

    if (access(spnavrcFilePath, F_OK) == 0) 
    {
        return true;
    } else 
    {
        return false;
    }
}

void restartDefaultSpacenavd(void) 
{
    if (spacenavdPid != 0) 
    {
        stopSpacenavd();
    }
    startDefaultSpacenavd();
}

void startSpacenavdWithSpnavrc(const char *spnavrcFilePath) 
{
    char *finalCommand = spacenavdWithSpnavrcStartScript;
    strcat(finalCommand, spnavrcFilePath);
}

void restartSpacenavdWithSpnavrc(const char *spnavrcFilePath) 
{
    if (spacenavdPid != 0) 
    {
        stopSpacenavd();
    }
    startSpacenavdWithSpnavrc(spnavrcFilePath);
}

void restartSpacenavd(const char* deviceName) 
{
        char buffer[128] = "\0";
        getcwd(buffer, sizeof buffer);
        strcat(buffer, "/spacenavd/spacenavd");

        char killCommand[256] = "ps -ef | grep './spacenavd/spacenavd' | grep -v grep | awk '{print $2}' | xargs -r kill -9";
        system(killCommand);
        sleep(1);
        if (isThereSpnavrcForDevice(deviceName)) 
        {
            char spnavrcFilePath[128] = "\0";
            strcat(spnavrcFilePath, spaceballSpnavrcDirectoryFilePath);
            strcat(spnavrcFilePath, deviceName);
            strcat(spnavrcFilePath, "/spnavrc");

            char finalCommand[256] = "./spacenavd/spacenavd";
            strcat(finalCommand, " -c '");
            strcat(finalCommand, spnavrcFilePath);
            strcat(finalCommand, "'");

            system(finalCommand);
        } else {
            char finalCommand[256] = "./spacenavd/spacenavd";
            system(finalCommand);
        }    
}

void printDeviceInfo(const char* deviceName) 
{
    char finalPath[128] = "\0";
    strcat(finalPath, spaceballInfoDirectoryFilePath);
    strcat(finalPath, deviceName);
    strcat(finalPath, ".txt");
    FILE *fp = fopen(finalPath, "r");
    if (fp == NULL) 
    {
        printf("Error: could not open file %s\n", finalPath);
        printf("Press any device button to start\n");
        return;
    }
    const unsigned MAX_LENGTH = 256;
    char buffer[MAX_LENGTH];
    while (fgets(buffer, MAX_LENGTH, fp)) 
    {
        printf("%s\n", buffer);
    }
    fclose(fp);
}

bool tryToPrintDevice() {
    buf[0] = "\0";
    if (spnav_dev_name(buf, sizeof buf) == -1) 
    {
        if (!isDemoStarted) sleep(1);
        if (spnav_dev_name(buf, sizeof buf) == -1) 
        {
            isPrintedAboutDevice = false;
            return false;
        }
    }
    if (isPrintedAboutDevice) return true;
    system("clear");
    printDeviceInfo(buf);
    isPrintedAboutDevice = true;
    isPrintedAboutConnectDevice = false;
    if (strcmp(prevDevice, buf) != 0) 
    {
        restartSpacenavd(buf);
        prevDevice[0] = "\0";
        strcpy(prevDevice, buf);
    }
    return true;
}


bool tryToPrintConnectDeviceMessage()
{
    if (isPrintedAboutConnectDevice) return true;
    system("clear");
    int i;
    for (i = 0; i < 20; ++i) {
            printf("\n");
        }
    for (i = 0; i < 5; ++i) {
            printf("\t");
    }
    printf("Connect your device...\n");
    isPrintedAboutConnectDevice = true;
    isPrintedAboutDevice = false;
    return false;
}

bool prepareForDemo()
{
    if(!(dpy = XOpenDisplay(0))) {
    		return false;
    	}
        
    	if(create_xwin("libspnav fly", 1080, 1080) == -1) {
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

void runDemo()
{
    isDemoStarted = true;
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
    					//goto end;
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
        isDemoStarted = false;
        glDeleteTextures(1, &grid_tex);
    	destroy_xwin();
    	spnav_close();
}

void openConnection()
{
    /* XXX: open connection to the spacenav driver */
        spnav_close();
       	while(spnav_open() == -1) {

        }
}

bool buttonWasPressed()
{
    ssock = spnav_fd();					/* libspnav socket */
    maxfd = xsock > ssock ? xsock : ssock;
    fd_set rdset;

    		/* XXX: add both sockets to the file descriptor set, to monitor both */
    		FD_ZERO(&rdset);
    		FD_SET(ssock, &rdset);

    		while(select(maxfd + 1, &rdset, 0, 0, 0) == -1 && errno == EINTR);


    		/* XXX: handle any pending spacenav events */
    		if(FD_ISSET(ssock, &rdset)) {
    			while(spnav_poll_event(&sev)) 
                {
    				if (sev.type == SPNAV_EVENT_BUTTON)
                    {
                        return true;
                    }
    			}
    		}
    
    return false;
}
int main(void)
{
    restartSpacenavd("dfdg");
    for (;;)
    {
        openConnection();
        spnav_dev_name(buf, sizeof buf);
        if (tryToPrintDevice())
        {
            if (buttonWasPressed())
            {
                prepareForDemo();
            } else
            {
                continue;
            }

        } else
        {
            tryToPrintConnectDeviceMessage();
            continue;
        }
        runDemo();
    }
    return 0;
}

void gen_textures(void)
{
	int i, j, r, g, b;
	static unsigned char pixels[128 * 128 * 3];
	unsigned char *pptr;

	pptr = pixels;
	for(i=0; i<128; i++) {
		float dy = abs(i - 64) / 64.0f;
		for(j=0; j<128; j++) {
			float dx = abs(j - 64) / 64.0f;
			float d = dx > dy ? dx : dy;
			float val = pow(d * 1.04f, 10.0) * 1.4f;

			r = (int)(214.0f * val);
			g = (int)(76.0f * val);
			b = (int)(255.0f * val);

			*pptr++ = r > 255 ? 255 : r;
			*pptr++ = g > 255 ? 255 : g;
			*pptr++ = b > 255 ? 255 : b;
		}
	}

	glGenTextures(1, &grid_tex);
	glBindTexture(GL_TEXTURE_2D, grid_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, 128, 128, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	pptr = pixels;
	for(i=0; i<128; i++) {
		int row = i >> 5;
		float dy = ((i - 12) & 0x1f) / 16.0f;
		for(j=0; j<128; j++) {
			int col = j >> 5;
			float dx = ((j - 12) & 0x1f) / 16.0f;
			float d = dx > dy ? dx : dy;
			int xor = (col ^ row) & 0xf;

			r = d < 0.5f ? (xor << 4) + 20 : 0;
			g = d < 0.5f ? (xor << 4) + 20 : 0;
			b = d < 0.5f ? (xor << 4) + 20 : 0;

			*pptr++ = r > 255 ? 255 : r;
			*pptr++ = g > 255 ? 255 : g;
			*pptr++ = b > 255 ? 255 : b;
		}
	}

	glGenTextures(1, &box_tex);
	glBindTexture(GL_TEXTURE_2D, box_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, 128, 128, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

void gen_scene(void)
{
	srand(0);
	scene = glGenLists(1);
	glNewList(scene, GL_COMPILE);


	glEnable(GL_TEXTURE_2D);


	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glTranslatef(0, 0, 0);	/* view matrix, push back to see the cube */
	switch (demoNumber) {
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

void moveObj(void);
void moveView(void);

void redraw(void)
{
    if (demoNumber != 3){
        moveObj();
	} else {
	    moveView();
	}
}

void moveObj(void) 
{
    float xform[16];

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0, 0, -20);	/* view matrix, push back to see the cube */

        /* XXX convert the accumulated position/rotation into a 4x4 view matrix */
        spnav_matrix_obj(xform, &posrot);
        glMultMatrixf(xform);		/* concatenate our computed view matrix */


        glCallList(scene);
        gen_scene();

        glXSwapBuffers(dpy, win);
}

void moveView(void) 
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

void reshape(int x, int y)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (float)x / (float)y, 1.0, 1000.0);
	glViewport(0, 0, x, y);
}

void handle_spnav_event(spnav_event *ev)
{
	switch(ev->type) {
	case SPNAV_EVENT_MOTION:
		/* XXX use the spnav_posrot_moveview utility function to modify the view
		 * position vector and orientation quaternion, based on the motion input
		 * we received.
		 *
		 * We'll also divide rotation values by two, to make rotation less
		 * sensitive. In this scale, it feels better that way in fly mode. The
		 * ideal ratio of sensitivities will vary depending on the scene scale.
		 */
		ev->motion.rx /= 2;
		ev->motion.ry /= 2;
		ev->motion.rz /= 2;
		if (demoNumber != 3) spnav_posrot_moveobj(&posrot, &ev->motion);
		else spnav_posrot_moveview(&posrot, &ev->motion);

		/* XXX: Drop any further pending motion events. This can make our input
		 * more responsive on slow or heavily loaded machines. We don't gain
		 * anything by processing a whole queue of relative motions.
		 */
		spnav_remove_events(SPNAV_EVENT_MOTION);
		redisplay_pending = 1;
		break;

	case SPNAV_EVENT_BUTTON:
		/* XXX reset position and orientation to identity on button presses to
		 * reset the view
		 */
		 if (sev.button.bnum == 0)
		 {
		     spnav_posrot_init(&posrot);
             redisplay_pending = 1;
		 } else if (sev.button.press){
            spnav_posrot_init(&posrot);
		    if (demoNumber == 3) demoNumber = 1;
		    else ++demoNumber;
		    gen_scene();
		    redraw();
		 }
		break;

	default:
		break;
	}
}

void draw_box(float xsz, float ysz, float zsz)
{
	xsz /= 2;
	ysz /= 2;
	zsz /= 2;

	glBegin(GL_QUADS);
	/* face +Z */
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0); glVertex3f(-xsz, -ysz, zsz);
	glTexCoord2f(1, 0); glVertex3f(xsz, -ysz, zsz);
	glTexCoord2f(1, 1); glVertex3f(xsz, ysz, zsz);
	glTexCoord2f(0, 1); glVertex3f(-xsz, ysz, zsz);
	/* face +X */
	glNormal3f(1, 0, 0);
	glTexCoord2f(0, 0); glVertex3f(xsz, -ysz, zsz);
	glTexCoord2f(1, 0); glVertex3f(xsz, -ysz, -zsz);
	glTexCoord2f(1, 1); glVertex3f(xsz, ysz, -zsz);
	glTexCoord2f(0, 1); glVertex3f(xsz, ysz, zsz);
	/* face -Z */
	glNormal3f(0, 0, -1);
	glTexCoord2f(0, 0); glVertex3f(xsz, -ysz, -zsz);
	glTexCoord2f(1, 0); glVertex3f(-xsz, -ysz, -zsz);
	glTexCoord2f(1, 1); glVertex3f(-xsz, ysz, -zsz);
	glTexCoord2f(0, 1); glVertex3f(xsz, ysz, -zsz);
	/* face -X */
	glNormal3f(-1, 0, 0);
	glTexCoord2f(0, 0); glVertex3f(-xsz, -ysz, -zsz);
	glTexCoord2f(1, 0); glVertex3f(-xsz, -ysz, zsz);
	glTexCoord2f(1, 1); glVertex3f(-xsz, ysz, zsz);
	glTexCoord2f(0, 1); glVertex3f(-xsz, ysz, -zsz);
	/* face +Y */
	glNormal3f(0, 1, 0);
	glTexCoord2f(0, 0);
	glVertex3f(-xsz, ysz, zsz);
	glVertex3f(xsz, ysz, zsz);
	glVertex3f(xsz, ysz, -zsz);
	glVertex3f(-xsz, ysz, -zsz);
	/* face -Y */
	glNormal3f(0, -1, 0);
	glTexCoord2f(0, 0);
	glVertex3f(-xsz, -ysz, -zsz);
	glVertex3f(xsz, -ysz, -zsz);
	glVertex3f(xsz, -ysz, zsz);
	glVertex3f(-xsz, -ysz, zsz);
	glEnd();
}

void genMyCubeScene() 
{
    glPolygonMode( GL_FRONT, GL_LINE );
    glBegin(GL_LINE_STRIP);
    int i = 0, j = 0, k = 0;
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
    int i, j;
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