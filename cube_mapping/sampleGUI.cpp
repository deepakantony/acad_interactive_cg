#include <iostream>

using namespace std;

#ifdef __APPLE__
#include <GL/glew.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <GLUT/glut.h>
#include <GLUI/glui.h>
#else
#include "GL/glew.h"
#include "GL/glut.h"
#include "GL/glui.h"
#endif

#include <math.h>

#include <Quad.cpp>
#include <cone.cpp>
#include <texture.c>

#define TEX_NAME_PLANK 1
#define TEX_NAME_EARTH 2
#define TEX_NAME_LIGHTMAP 3
#define TEX_NAME_CUBEMAP 4

//#define TEX_IMAGE_PLANK "NewPlank.rgb"
//#define TEX_IMAGE_EARTH "eoe3.rgb"
//#define TEX_IMAGE_LIGHTMAP "lightmap.rgb"
char *tex_image_plank, *tex_image_earth, *tex_image_lightmap;

static GLuint texPlank, texEarth, texLightmap, texCubemap;

// the quadric object
GLUquadric *quadric;

// texture coordinates for the floor/Plank
float tcPlank1[2] = {0,0};
float tcPlank2[2] = {4,0};
float tcPlank3[2] = {4,4};
float tcPlank4[2] = {0,4};

// vertices for the quads of the room
float v1[3] = {5.0, 0.0, 5.0};
float v2[3] = {5.0, 10.0, 5.0};
float v3[3] = {-5.0, 10.0, 5.0}; 
float v4[3] = {-5.0, 0.0, 5.0};
float v5[3] = {-5.0, 0.0, -5.0};
float v6[3] = {-5.0, 10.0, -5.0};
float v7[3] = {5.0, 10.0, -5.0};
float v8[3] = {5.0, 0.0, -5.0};

// cone parameters
#define CONE_HEIGHT 3.0
#define CONE_RADIUS 2.0
#define CONE_RES 200.0
// indicates how many times the earth should be displayed
#define CONE_TEXTURE_T 1.0 
#define CONE_TEXTURE_S 2.0 

const int redBlend[] = {1, 0, 0, 1};

// cube map variables
#define SPHERE_RADIUS 2.0
#define SPHERE_CENTER 5.0
GLenum cubemap[6];
const int viewportWidth = 64;
const int viewportHeight = 64;
GLvoid *pixels;
const int textureTargets[] = {
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};
static bool initializedCM = false;
float modelviewINV[16];

// environment materials
Quad *wallFront, *wallBack, *wallRight, *wallLeft, *roomRoof, *roomFloor;
Cone *cone;

// let there be LIGHT
float lightAmbR = 0.1, lightAmbG = 0.1, lightAmbB = 0.1;
float lightDifR = 1.0, lightDifG = 1.0, lightDifB = 1.0;
float lightSpecR = 1.0, lightSpecG = 1.0, lightSpecB = 1.0;
GLfloat lightPosition[] = {15.0, 15.0, 15.0, 1.0 };
GLUI_Spinner *spin_s1, *spin_s2, *spin_s3, *spin_s4, *spin_s5, *spin_s6, *spin_s7, *spin_s8, *spin_s9;
int	main_window;

// the camera info  
float eye[3];
float lookat[3];

// initial camera info
float initEye[3];
float initLookat[3];

// pointers for all of the glui controls
GLUI *glui;
GLUI_Rollout		*object_rollout;
GLUI_RadioGroup		*object_type_radio;
GLUI_Rotation		*object_rotation;
GLUI_Translation	*object_xz_trans;
GLUI_Translation	*lightmapTranslation;
GLUI_Translation	*object_y_trans;

GLUI_Rollout		*anim_rollout;
GLUI_Button			*action_button;

GLUI_Checkbox *enableLightMapCheck;
GLUI_Checkbox *draw_object_check;

// This  checkbox utilizes the callback
GLUI_Checkbox *use_depth_buffer;


// the user id's that we can use to identify which control
// caused the callback to be called
#define CB_DEPTH_BUFFER 0
#define CB_RESET_LIGHT_BUTTON 1
#define CB_RESET 2
#define CB_UPDATE_CUBEMAP 3

// walking action variables
//
GLfloat step = 0;
GLfloat live_anim_speed = 3;

// live variables
// each of these are associated with a control in the interface.
// when the control is modified, these variables are automatically updated
int coneTexturingType;	// 0=cube, 1=sphere, 2=torus
float live_object_rotation[16];
float live_object_xz_trans[2];
float lightmapMovement[2];
float lightmapMovementPrev[2] = {0.0,0.0};
float live_object_y_trans;
int enableLightMap;
int enableCubeMapping;
int enableCone;
int live_use_depth_buffer;

// Functions
void drawCone();
void drawRoomWithCone();
void light();

void getRotationMatrix(float rMat[16]) {
	float modelview[16], b[3][3], inv[3][3];

	int i, j, k;

	float multFactor;

	// initialize final matrix
	for(i=0; i<15; i++)
		rMat[i] = 0;
	rMat[15] = 1;

	// initialize inverse matrix to identity matrix
	for(i=0; i<3; i++){
		for(j=0;j<3;j++)
			inv[i][j] = 0;
		inv[i][i] = 1;
	}

	glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

	// get only the rotation part
	for(i=0,k=0; i<12; i+=4,k++)
		for(j=0; j<3; j++)
			b[j][k] = modelview[i+j];

	//free(modelview);

	// find the inverse
	for(i=0; i<3; i++) {
		// divide the i'th row by [i,i] so that [i,i]'th term is 1
		multFactor = b[i][i];
		for(j = 0; j<3; j++) {
			b[i][j] = b[i][j]/multFactor;
			inv[i][j] = inv[i][j]/multFactor;
		}

		// clear up i'th column
		for(j = 0; j<3; j++)
			if(i!=j) {
				multFactor = b[j][i];
				for(k=0; k<3; k++){
					b[j][k] = b[j][k] - multFactor*b[i][k];
					inv[j][k] = inv[j][k] - multFactor*inv[i][k];
				}
			}
	}

	// fill the final inversed rotation matrix
	for(i=0,k=0; i<12; i+=4,k++)
		for(j=0; j<3; j++)
			rMat[j+i] = inv[j][k];
}

void updateGluLookAt(int lookAt) {
	switch(lookAt) {
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
		gluLookAt(0.0, SPHERE_CENTER, 0.0, 1.0, SPHERE_CENTER, 0.0, 0.0, 1.0, 0.0);
		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
		gluLookAt(0.0, SPHERE_CENTER, 0.0, -1.0, SPHERE_CENTER, 0.0, 0.0, 1.0, 0.0);		
		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
		gluLookAt(0.0, SPHERE_CENTER, 0.0, 0.0, SPHERE_CENTER+1.0, 0.0, 0.0, 0.0, 1.0);
		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
		gluLookAt(0.0, SPHERE_CENTER, 0.0, 0.0, SPHERE_CENTER-1.0, 0.0, 0.0, 0.0, -1.0);
		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
		gluLookAt(0.0, SPHERE_CENTER, 0.0, 0.0, SPHERE_CENTER, 1.0, 0.0, 1.0, 0.0);
		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		gluLookAt(0.0, SPHERE_CENTER, 0.0, 0.0, SPHERE_CENTER, -1.0, 0.0, 1.0, 0.0);
		break;
	}
}

void initCubeMap() {
	//save viewport and set up new one
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT,(int*)viewport);

	glViewport(0,0,viewportWidth,viewportHeight);
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective ( 90.0, 1.0, 1.0, 100.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glGenTextures(1, &texCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texCubemap);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	for(int i = 0; i < 6; i++) {
		glPushMatrix();
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			updateGluLookAt(textureTargets[i]);
			glViewport(0,0,viewportWidth,viewportHeight);
			drawRoomWithCone();
			glCopyTexImage2D(textureTargets[i], 0, GL_RGBA, 0, 0, viewportWidth, viewportHeight, 0);
		glPopMatrix();
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
}

void transpose(float mat[16], float transpose[16]) {
	int t = 0;
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 16; j = j+4)
			transpose[t++] = mat[i+j];
	}
}

void cubeMapSphere() {
	if(!initializedCM){
		initCubeMap();
		initializedCM = true;
	}

	glDisable(GL_BLEND);
	glColor3f(1.0,1.0,1.0);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texCubemap);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glPushMatrix();
		glTranslatef(0.0, SPHERE_CENTER, 0.0);

		glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			getRotationMatrix(modelviewINV);
			glMultMatrixf(modelviewINV);


		glMatrixMode(GL_MODELVIEW);

		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);

		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_R);

		glutSolidSphere(SPHERE_RADIUS, 30, 30);
	glPopMatrix();

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);

	glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_CUBE_MAP);

	glEnable(GL_BLEND);
}

void normalize(float v[3])
{
	float l = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	l = 1 / (float)sqrt(l);

	v[0] *= l;
	v[1] *= l;
	v[2] *= l;
}

void crossproduct(float a[3], float b[3], float res[3])
{
	res[0] = (a[1] * b[2] - a[2] * b[1]);
	res[1] = (a[2] * b[0] - a[0] * b[2]);
	res[2] = (a[0] * b[1] - a[1] * b[0]);
}

float length(float v[3])
{
	return (float)sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}


void myGlutIdle(void)
{
	// make sure the main window is active
	if (glutGetWindow() != main_window)
		glutSetWindow(main_window);

	// if you have moving objects, you can do that here

	// just keep redrawing the scene over and over
	glutPostRedisplay();
}


// mouse handling functions for the main window
// left mouse translates, middle zooms, right rotates
// keep track of which button is down and where the last position was
int cur_button = -1;
int last_x;
int last_y;

// catch mouse up/down events
void myGlutMouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
		cur_button = button;
	else
	{
		if (button == cur_button)
			cur_button = -1;
	}

	last_x = x;
	last_y = y;
}

// catch mouse move events
void myGlutMotion(int x, int y)
{
	// the change in mouse position
	int dx = x-last_x;
	int dy = y-last_y;

	float scale, len, theta;
	float neye[3], neye2[3];
	float f[3], r[3], u[3];

	switch(cur_button)
	{
	case GLUT_LEFT_BUTTON:
		// translate
		f[0] = lookat[0] - eye[0];
		f[1] = lookat[1] - eye[1];
		f[2] = lookat[2] - eye[2];
		u[0] = 0;
		u[1] = 1;
		u[2] = 0;

		// scale the change by how far away we are
		scale = sqrt(length(f)) * 0.007;

		crossproduct(f, u, r);
		crossproduct(r, f, u);
		normalize(r);
		normalize(u);

		eye[0] += -r[0]*dx*scale + u[0]*dy*scale;
		eye[1] += -r[1]*dx*scale + u[1]*dy*scale;
		eye[2] += -r[2]*dx*scale + u[2]*dy*scale;

		lookat[0] += -r[0]*dx*scale + u[0]*dy*scale;
		lookat[1] += -r[1]*dx*scale + u[1]*dy*scale;
		lookat[2] += -r[2]*dx*scale + u[2]*dy*scale;

		break;

	case GLUT_MIDDLE_BUTTON:
		// zoom
		f[0] = lookat[0] - eye[0];
		f[1] = lookat[1] - eye[1];
		f[2] = lookat[2] - eye[2];

		len = length(f);
		normalize(f);

		// scale the change by how far away we are
		len -= sqrt(len)*dx*0.03;

		eye[0] = lookat[0] - len*f[0];
		eye[1] = lookat[1] - len*f[1];
		eye[2] = lookat[2] - len*f[2];

		// make sure the eye and lookat points are sufficiently far away
		// push the lookat point forward if it is too close
		if (len < 1)
		{
			printf("lookat move: %f\n", len);
			lookat[0] = eye[0] + f[0];
			lookat[1] = eye[1] + f[1];
			lookat[2] = eye[2] + f[2];
		}

		break;

	case GLUT_RIGHT_BUTTON:
		// rotate

		neye[0] = eye[0] - lookat[0];
		neye[1] = eye[1] - lookat[1];
		neye[2] = eye[2] - lookat[2];

		// first rotate in the x/z plane
		theta = -dx * 0.007;
		neye2[0] = (float)cos(theta)*neye[0] + (float)sin(theta)*neye[2];
		neye2[1] = neye[1];
		neye2[2] =-(float)sin(theta)*neye[0] + (float)cos(theta)*neye[2];


		// now rotate vertically
		theta = -dy * 0.007;

		f[0] = -neye2[0];
		f[1] = -neye2[1];
		f[2] = -neye2[2];
		u[0] = 0;
		u[1] = 1;
		u[2] = 0;
		crossproduct(f, u, r);
		crossproduct(r, f, u);
		len = length(f);
		normalize(f);
		normalize(u);

		neye[0] = len * ((float)cos(theta)*f[0] + (float)sin(theta)*u[0]);
		neye[1] = len * ((float)cos(theta)*f[1] + (float)sin(theta)*u[1]);
		neye[2] = len * ((float)cos(theta)*f[2] + (float)sin(theta)*u[2]);

		eye[0] = lookat[0] - neye[0];
		eye[1] = lookat[1] - neye[1];
		eye[2] = lookat[2] - neye[2];

		break;
	}


	last_x = x;
	last_y = y;

	glutPostRedisplay();
}

// you can put keyboard shortcuts in here
void myGlutKeyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
	// quit
	case 27: 
	case 'q':
	case 'Q':
		exit(0);
		break;
	}

	glutPostRedisplay();
}

// the window has changed shapes, fix ourselves up
void myGlutReshape(int	x, int y)
{
	int tx, ty, tw, th;
	GLUI_Master.get_viewport_area(&tx, &ty, &tw, &th);
	glViewport(tx, ty, tw, th);

	glutPostRedisplay();
}

void drawCone(){
	switch (coneTexturingType) {
		case 0:
			cone->drawWithTexture(texEarth, CONE_TEXTURE_S, CONE_TEXTURE_T, GL_DECAL);
			break;
		case 1:
			cone->drawWithTexture(texEarth, CONE_TEXTURE_S, CONE_TEXTURE_T, GL_MODULATE);
			break;
		case 2:
			cone->drawWithTexture(texEarth, CONE_TEXTURE_S, CONE_TEXTURE_T, GL_REPLACE);
			break;
		case 3:
			//glPushAttrib(
			glTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, redBlend);
			cone->drawWithTexture(texEarth, CONE_TEXTURE_S, CONE_TEXTURE_T, GL_BLEND);
			break;
	}
}

void drawRoomWithCone() {
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glColor3f(1.0, 0.0, 1.0);
	wallFront->draw();
	glColor3f(1.0, 0.2, 0.2);
	wallBack->draw();
	glColor3f(0.0, 0.0, 1.0);
	wallRight->draw();
	glColor3f(0.0, 1.0, 1.0);
	wallLeft->draw();
	glColor3f(0.7, 0.3, 0.3);
	roomRoof->draw();

	if(enableLightMap) {
		if(lightmapMovement[0] > 8.0*0.05) lightmapMovement[0] = 8*0.05;
		else if(lightmapMovement[0] < -8.0*0.05) lightmapMovement[0] = -8.0*0.05;
		if(lightmapMovement[1] > 8.0*0.05) lightmapMovement[1] = 8.0*0.05;
		else if(lightmapMovement[1] < -8.0*0.05) lightmapMovement[1] = -8.0*0.05;
		lightmapTranslation->set_x(lightmapMovement[0]);
		lightmapTranslation->set_y(lightmapMovement[1]);
		roomFloor->drawWithMultiTexture(texPlank, texLightmap, lightmapMovement);
		lightmapMovementPrev[0] = lightmapMovement[0];
		lightmapMovementPrev[1] = lightmapMovement[1];
	}
	else {
		lightmapMovement[0] = lightmapMovementPrev[0];
		lightmapMovement[1] = lightmapMovementPrev[1];
		lightmapTranslation->set_x(lightmapMovementPrev[0]);
		lightmapTranslation->set_y(lightmapMovementPrev[1]);
		roomFloor->drawWithTexture(texPlank);
	}

	if(enableCone) {
		glCullFace(GL_FRONT);
		drawCone();

		glCullFace(GL_BACK);
		drawCone();
	}

	glDisable(GL_CULL_FACE);
}

// draw the scene
void myGlutDisplay(	void )
{
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// projection transform
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1, 1, -1, 1, 1, 1000);

	// camera transform
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	light();
	gluLookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], 0, 1, 0);
	
	glPushMatrix();
		glTranslatef(live_object_xz_trans[0], live_object_y_trans, -live_object_xz_trans[1]);
		glMultMatrixf(live_object_rotation);
		
		if(enableCubeMapping) cubeMapSphere();
		drawRoomWithCone();
		
		//if(enableCubeMapping) drawQuad();

	glPopMatrix();
	glutSwapBuffers(); 
	
}

void initRoom() {
	wallFront = new Quad(v4, v3, v2, v1);
	wallLeft = new Quad(v5, v6, v3, v4);
	wallBack = new Quad(v8, v7, v6, v5);
	wallRight = new Quad(v1, v2, v7, v8);
	roomFloor = new Quad(v8, v5, v4, v1);
	roomRoof = new Quad(v3, v6, v7, v2);

	cone = new Cone( CONE_RADIUS, CONE_HEIGHT, CONE_RES);
	roomFloor->initTexture(tcPlank1, tcPlank2, tcPlank3, tcPlank4);
}

void resetLight() {
	//lightAmbR = .0, lightAmbG = 0.0, lightAmbB = 0.0;
	lightDifR = 1.0, lightDifG = 1.0, lightDifB = 1.0;
	lightSpecR = 1.0, lightSpecG = 1.0, lightSpecB = 1.0;
}

void resetLightSpinners() {
	//spin_s1->set_float_val(lightAmbR);
	//spin_s2->set_float_val(lightAmbG);
	//spin_s3->set_float_val(lightAmbB);
	spin_s4->set_float_val(lightDifR);
	spin_s5->set_float_val(lightDifG);
	spin_s6->set_float_val(lightDifB);
	//spin_s7->set_float_val(lightSpecR);
	//spin_s8->set_float_val(lightSpecG);
	//spin_s9->set_float_val(lightSpecB);
}

// initialize the light
void light() {
	GLfloat lightAmbient[] = {lightAmbR, lightAmbG, lightAmbB, 1.0};
	GLfloat lightDiffuse[] = {lightDifR, lightDifG, lightDifB, 1.0};
	GLfloat lightSpecular[] = {lightSpecR, lightSpecG, lightSpecB, 1.0};

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

// initialize textures
void initTextures() {
	static unsigned *image;
	static int width, height, components;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// initializing the plank
	glGenTextures(1, &texPlank);
	glBindTexture(GL_TEXTURE_2D, texPlank);
    
	image = read_texture(tex_image_plank, &width, &height, &components);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, components, width,
		height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		image);
	gluBuild2DMipmaps(GL_TEXTURE_2D, components, width, height, GL_RGBA, 
		GL_UNSIGNED_BYTE, image);

	free(image);

	// initializing the earth
	glGenTextures(1, &texEarth);
	glBindTexture(GL_TEXTURE_2D, texEarth);
    
	image = read_texture(tex_image_earth, &width, &height, &components);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, components, width,
		height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		image);

	free(image);
	
	// initializing the light map
	glGenTextures(1, &texLightmap);
	glBindTexture(GL_TEXTURE_2D, texLightmap);
    
	image = read_texture(tex_image_lightmap, &width, &height, &components);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, components, width,
		height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		image);

	free(image);
}

// some controls generate a callback when they are changed
void glui_cb(int control)
{

  switch(control)
    {
    case CB_DEPTH_BUFFER:
      if (live_use_depth_buffer)
	glEnable(GL_DEPTH_TEST);
      else
	glDisable(GL_DEPTH_TEST);
      break;
    case CB_RESET_LIGHT_BUTTON:
      resetLight();
      resetLightSpinners();
      break;
    case CB_RESET:
      object_rotation->reset();
      live_object_xz_trans[0] = 0;
      live_object_xz_trans[1] = 0;
      live_object_y_trans = 0;
      // initialize the camera
      eye[0] = 0;
      eye[1] = 4;
      eye[2] = 15;
      lookat[0] = 0;
      lookat[1] = 4;
      lookat[2] = 0;
      break;
    case CB_UPDATE_CUBEMAP:
      initializedCM = false;
      break;
    }

  glutPostRedisplay();
}

// entry point
int main(int argc, char* argv[])
{
  if(argc == 4) {
    tex_image_plank = argv[1];
    tex_image_earth = argv[2];
    tex_image_lightmap = argv[3];
    cout << tex_image_plank << endl;
    cout << tex_image_earth << endl;
    cout << tex_image_lightmap << endl;
  }
  else {
    cerr << "Usage: <program> <texture1_plank> <texture2_earth> <texture3_lightmap>" << endl;
    return -1;
  }

  //
  // create the glut window
  //
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH);
  glutInitWindowSize(1200, 900);
  glutInitWindowPosition(20,20);
  main_window = glutCreateWindow("Sample Interface");


  glewInit();

  //
  // set callbacks
  //
  glutDisplayFunc(myGlutDisplay);
  GLUI_Master.set_glutReshapeFunc(myGlutReshape);
  GLUI_Master.set_glutIdleFunc(myGlutIdle);
  GLUI_Master.set_glutKeyboardFunc(myGlutKeyboard);
  GLUI_Master.set_glutMouseFunc(myGlutMouse);
  glutMotionFunc(myGlutMotion);

  initRoom();

  //
  // create the interface subwindow and add widgets
  //
  glui = GLUI_Master.create_glui_subwindow(main_window, GLUI_SUBWINDOW_LEFT);


  // initialize live variables
  live_object_xz_trans[0] = 0;
  live_object_xz_trans[1] = 0;
  live_object_y_trans = 0;
  lightmapMovement[0] = 0;
  lightmapMovement[1] = 0;
  enableLightMap = 0;
  enableCubeMapping = 0;
  enableCone = 0;
  live_use_depth_buffer = 1;

  // quit button
  glui->add_button("Quit", 0, (GLUI_Update_CB)exit);

  // empty space
  glui->add_statictext("");

  // the object rollout
  object_rollout = glui->add_rollout("Object");

  // rotation and translation controls
  // we need an extra panel to keep things grouped properly
  GLUI_Panel *transform_panel = glui->add_panel_to_panel(object_rollout, "", GLUI_PANEL_NONE);
  object_rotation = glui->add_rotation_to_panel(transform_panel, "Rotation", live_object_rotation);
  object_rotation->reset();

  glui->add_column_to_panel(transform_panel, false);
  object_xz_trans = glui->add_translation_to_panel(transform_panel, "Translate XZ", 
						   GLUI_TRANSLATION_XY, live_object_xz_trans);

  glui->add_column_to_panel(transform_panel, false);
  object_y_trans =  glui->add_translation_to_panel(transform_panel, "Translate Y", 
						   GLUI_TRANSLATION_Y, &live_object_y_trans);

  object_xz_trans->scale_factor = 0.1f;
  object_y_trans->scale_factor = 0.1f;

  glui->add_button_to_panel(object_rollout, "Reset Object Transform", CB_RESET, glui_cb);

  // empty space
  glui->add_statictext("");

  // the object rollout
  object_rollout = glui->add_rollout("Cone texturing");
	
  glui->add_checkbox_to_panel(object_rollout, "Draw Cone", &enableCone);
  // the radio buttons
  object_type_radio = glui->add_radiogroup_to_panel(object_rollout, &coneTexturingType);
  glui->add_radiobutton_to_group(object_type_radio, "Decal");
  glui->add_radiobutton_to_group(object_type_radio, "Modulate");
  glui->add_radiobutton_to_group(object_type_radio, "Replace");
  glui->add_radiobutton_to_group(object_type_radio, "Blend");

  // the walk control
  anim_rollout = glui->add_rollout("Light Intensity");
  //spin_s1 = glui->add_spinner_to_panel(anim_rollout, "Ambient Red", GLUI_SPINNER_FLOAT, &lightAmbR);
  //spin_s1->set_float_limits(0.0, 1.0);
  //spin_s2 = glui->add_spinner_to_panel(anim_rollout, "Ambient Green", GLUI_SPINNER_FLOAT, &lightAmbG);
  //spin_s2->set_float_limits(0.0, 1.0);
  //spin_s3 = glui->add_spinner_to_panel(anim_rollout, "Ambient Blue", GLUI_SPINNER_FLOAT, &lightAmbB);
  //spin_s3->set_float_limits(0.0, 1.0);
  spin_s4 = glui->add_spinner_to_panel(anim_rollout, "Diffuse Red", GLUI_SPINNER_FLOAT, &lightDifR);
  spin_s4->set_float_limits(0.0, 1.0);
  spin_s5 = glui->add_spinner_to_panel(anim_rollout, "Diffuse Green", GLUI_SPINNER_FLOAT, &lightDifG);
  spin_s5->set_float_limits(0.0, 1.0);
  spin_s6 = glui->add_spinner_to_panel(anim_rollout, "Diffuse Blue", GLUI_SPINNER_FLOAT, &lightDifB);
  spin_s6->set_float_limits(0.0, 1.0);
  //spin_s7 = glui->add_spinner_to_panel(anim_rollout, "Specular Red", GLUI_SPINNER_FLOAT, &lightSpecR);
  //spin_s7->set_float_limits(0.0, 1.0);
  //spin_s8 = glui->add_spinner_to_panel(anim_rollout, "Specular Red", GLUI_SPINNER_FLOAT, &lightSpecG);
  //spin_s8->set_float_limits(0.0, 1.0);
  //spin_s9 = glui->add_spinner_to_panel(anim_rollout, "Specular Red", GLUI_SPINNER_FLOAT, &lightSpecB);
  //spin_s9->set_float_limits(0.0, 1.0);

  action_button = glui->add_button_to_panel(anim_rollout, "Reset Light Intensity", CB_RESET_LIGHT_BUTTON, glui_cb);

  // the object rollout
  object_rollout = glui->add_rollout("Light Map");
	
  glui->add_checkbox_to_panel(object_rollout, "Lightmap", &enableLightMap);
  lightmapTranslation = glui->add_translation_to_panel(object_rollout, "Translate Lighmap", 
						       GLUI_TRANSLATION_XY, lightmapMovement);
  lightmapTranslation->scale_factor = 0.05f;

  // the object rollout
  object_rollout = glui->add_rollout("Cube Map");
  glui->add_checkbox_to_panel(object_rollout,"Cube mapped Sphere", &enableCubeMapping);
  glui->add_button_to_panel(object_rollout, "Update Cubemap", CB_UPDATE_CUBEMAP, glui_cb);


  // empty space
  glui->add_statictext("");

  //glui->add_checkbox("Use Depth Buffer", &live_use_depth_buffer, CB_DEPTH_BUFFER, glui_cb);

  glui->set_main_gfx_window(main_window);
  
  // initialize the camera
  initEye[0] = eye[0] = 0;
  initEye[1] = eye[1] = 4;
  initEye[2] = eye[2] = 15;
  initLookat[0] = lookat[0] = 0;
  initLookat[1] = lookat[1] = 4;
  initLookat[2] = lookat[2] = 0;

  // initialize gl
  //glEnable(GL_BLEND);
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_COLOR_MATERIAL);
  initTextures();

  quadric  = gluNewQuadric();
  // initialize glu
  gluQuadricTexture(quadric, GL_TRUE);
  gluQuadricDrawStyle(quadric, GLU_FLAT);

  // give control over to glut
  glutMainLoop();

  return 0;
}
