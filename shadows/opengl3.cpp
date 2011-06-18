#include "GL/glew.h"

#ifdef __APPLE__
#include "GLUT/glut.h"
#include "GLUI/glui.h"
#else
#include "GL/glut.h"
#include "GL/glui.h"
#endif

#include <math.h>

#include "Quad.cpp"
#include "cone.cpp"
#include "texture.c"

#include <iostream>

using namespace std;

//window size
int windowWidth=1200, windowHeight=900;

// shadow types
#define NO_SHADOWS 0
#define SHADOW_PROJECTIVE 1
#define SHADOW_MAPPING 2
#define SHADOW_VOLUMES 3
int liveShadowType = NO_SHADOWS;

// Colors
const float white[] = {1.0, 1.0, 1.0, 1.0};
const float dimWhite[] = {0.2, 0.2, 0.2, 1.0};
const float black[] = {0.0, 0.0, 0.0, 1.0};

// point light(sphere) identifiers
#define PLIGHT_SPHERE_RADIUS 0.3
#define PLIGHT_SPHERE_Y 9.6
#define PLIGHT_SPHERE_Z 0.1
static float lightPos[4] = {0,PLIGHT_SPHERE_Y,PLIGHT_SPHERE_Z,0};

/* shadow map vars */
static const float vpShadowMap = 512;
static const float lightFOV = 90, lightAspect = 1.0, lightNearPlane = 0.1, lightFarPlane = 50;
//Matrices
float lightProjectionMatrix[16], lightViewMatrix[16];
float cameraProjectionMatrix[16], cameraViewMatrix[16];

// shadow volume vars
struct surface {
	float vertices[4][3];
};

struct cube {
	struct surface *surfaces[6];
	float position[3];
};
#define OCCLUDER_SIZE 0.5
const float occluderPosition[3] = {0.0, PLIGHT_SPHERE_Y-2.0, PLIGHT_SPHERE_Z};

// tori identifiers
#define TORUS_IR 0.3
#define TORUS_OR 1.0
static const float toriLoc[] = {2.0, 6.0, 2.0};

#define TEX_NAME_PLANK 1
#define TEX_NAME_EARTH 2
#define TEX_NAME_LIGHTMAP 3
#define TEX_NAME_CUBEMAP 4

//#define TEX_IMAGE_PLANK "NewPlank.rgb"
//#define TEX_IMAGE_EARTH "eoe3.rgb"
//#define TEX_IMAGE_LIGHTMAP "lightmap.rgb"
char *TEX_IMAGE_PLANK, *TEX_IMAGE_LIGHTMAP, *TEX_IMAGE_EARTH;

static GLuint texPlank, texEarth, texLightmap, texCubemap, texShadowMap;

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
#define SPHERE_RADIUS 1.5
#define SPHERE_CENTER 4.5
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
float lightDif = 1.0;// lightDifG = 1.0, lightDifB = 1.0;
float lightSpecR = 1.0, lightSpecG = 1.0, lightSpecB = 1.0;
GLfloat lightPosition[] = {15.0, 15.0, 15.0, 1.0 };
GLUI_Spinner *spin_s1, *spin_s2, *spin_s3, *spin_s4, *spin_s5, *spin_s6, *spin_s7, *spin_s8, *spin_s9;
int	main_window;

// the camera info  
float eye[3];
float lookat[3];
float up[3];

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
GLUI_Translation* lightYTranslation;
GLUI_Translation* lightTranslation;

GLUI_Rollout		*anim_rollout;
GLUI_Button			*action_button;

GLUI_Checkbox *enableLightMapCheck;
GLUI_Checkbox *draw_object_check;

GLUI_Rotation           *cubeRotation;
GLUI_Translation        *cubeXZTrans;
GLUI_Translation        *cubeYTrans;

float liveCubeRotation[16];
float liveCubeXZTrans[2];
float liveCubeYTrans;

// This  checkbox utilizes the callback
GLUI_Checkbox *use_depth_buffer;


// the user id's that we can use to identify which control
// caused the callback to be called
#define CB_DEPTH_BUFFER 0
#define CB_RESET_LIGHT_BUTTON 1
#define CB_RESET 2
#define CB_UPDATE_CUBEMAP 3
#define CB_RESET_LIGHT_POS 4

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
float liveLightMovement[2];
float liveLightYMovement;
float live_object_y_trans;
int enableLightMap;
int enableCubeMapping;
int enableCone;
int live_use_depth_buffer;

// Functions
void drawRoom();
void drawCone();
void drawRoom();
void initLight();
void cubeMapSphere();
void drawTori();
void drawLight();
void transpose(float mat[16], float transpose[16]);
void normalize(float v[3]);
void drawConeNoTex();
void cubeMapSphereNoTex();
struct cube *newCube(float size);
void renderCube(struct cube *c);
void drawLight();

void cubeMapSphereNoTex() {
	glPushMatrix();
		glTranslatef(0.0, SPHERE_CENTER, 0.0);
		glutSolidSphere(SPHERE_RADIUS, 50, 50);
	glPopMatrix();
}

void drawConeNoTex(){
	glPushMatrix();
		glRotatef(90, -1.0, 0.0, 0.0);
		glutSolidCone(CONE_RADIUS, CONE_HEIGHT, 100, 100);
	glPopMatrix();
}

void drawRoomObjects() {
	if(enableCone) drawCone();
	if(enableCubeMapping) cubeMapSphere();
	glColor3f(1.0, 1.0, 0.0);
	drawTori();
}

void drawScene() {
	drawRoom();
	drawRoomObjects();
}

void drawRoomObjectNoColor() {
	if(enableCone) drawCone();
	if(enableCubeMapping) cubeMapSphere();
	drawTori();
}

void drawRoomObjectsNoTex() {
	if(enableCone) drawConeNoTex();
	if(enableCubeMapping) cubeMapSphereNoTex();
	drawTori();
}

// draw tori
void drawTori() {
	glPushMatrix();
		glTranslatef(toriLoc[0],toriLoc[1],toriLoc[2]);
		glutSolidTorus(TORUS_IR, TORUS_OR, 100, 100);
		glTranslatef(TORUS_OR - TORUS_IR/2, 0.0, 0.0);
		glRotatef(90, 1.0, 0.0, 0.0);
		glutSolidTorus(TORUS_IR, TORUS_OR, 100, 100);
	glPopMatrix();
}

// draw the point light
void drawLight() {
	glPushMatrix();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0,1.0,1.0, 1.0);
		GLfloat matEmission[] = {lightDif,lightDif,lightDif,1.0};
		glMaterialfv(GL_FRONT, GL_EMISSION, matEmission);
		glTranslatef( lightPos[0], lightPos[1], lightPos[2]);
		glutSolidSphere(PLIGHT_SPHERE_RADIUS, 30, 30);
		GLfloat defaultMatEmission[] = {0.0, 0.0, 0.0, 1.0};
		glMaterialfv(GL_FRONT, GL_EMISSION, defaultMatEmission);
		glDisable(GL_BLEND);
	glPopMatrix();
}

// initialize the light
void initLight() {
	lightPos[0] = liveLightMovement[0];
	lightPos[1] = liveLightYMovement;
	lightPos[2] = -liveLightMovement[1];
	lightPos[3] = 0.0;

	GLfloat lightAmbient[] = {lightAmbR, lightAmbG, lightAmbB, 1.0};
	GLfloat lightDiffuse[] = {lightDif, lightDif, lightDif, 1.0};
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

void findEquation(float v1[3],float v2[3],float v3[3],float eq[4]) {
	eq[0] = v3[1]*(v1[2]-v2[2])+v1[1]*(v2[2]-v3[2])+v2[1]*(v3[2]-v1[2]);
	eq[1] = v3[2]*(v1[0]-v2[0])+v1[2]*(v2[0]-v3[0])+v2[2]*(v3[0]-v1[0]);
	eq[2] = v3[0]*(v1[1]-v2[1])+v1[0]*(v2[1]-v3[1])+v2[0]*(v3[1]-v1[1]);
	eq[3] = -(v3[0]*(v1[1]*v2[2]-v2[1]*v1[2])+v1[0]*(v2[1]*v3[2]-v3[1]*v2[2])+v2[0]*(v3[1]*v1[2]-v1[1]*v3[2]));
}

bool isVertexAbovePlane(float v[3], float eq[4]) {
	return ((eq[0]*v[0]+eq[1]*v[1]+eq[2]*v[2]+eq[3])>0)?false:true;
}

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
			drawRoom();
			if(enableCone)drawCone();
			if(liveShadowType) {
				static struct cube *occluder;
				occluder = newCube(OCCLUDER_SIZE);
				occluder->position[0] = occluderPosition[0]+liveCubeXZTrans[0];
				occluder->position[1] = occluderPosition[1]+liveCubeYTrans;
				occluder->position[2] = occluderPosition[2]+liveCubeXZTrans[1];

				renderCube(occluder);
			}
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

		/*neye[0] = eye[0] - lookat[0];
		neye[1] = eye[1] - lookat[1];
		neye[2] = eye[2] - lookat[2];*/
		neye[0] = -eye[0] + lookat[0];
		neye[1] = -eye[1] + lookat[1];
		neye[2] = -eye[2] + lookat[2];

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

		/*eye[0] = lookat[0] - neye[0];
		eye[1] = lookat[1] - neye[1];
		eye[2] = lookat[2] - neye[2];*/

		lookat[0] = eye[0] - neye[0];
		lookat[1] = eye[1] - neye[1];
		lookat[2] = eye[2] - neye[2];

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
	windowWidth = tx; windowHeight = ty;

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

void drawRoomSM() {
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
	roomFloor->drawWithMultiTexture(texPlank, texShadowMap);
	glDisable(GL_CULL_FACE);
}

void drawRoomNoTex() {
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
	roomFloor->draw();
	glDisable(GL_CULL_FACE);
}

void drawRoomNoColor() {
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	wallFront->draw();
	wallBack->draw();
	wallRight->draw();
	wallLeft->draw();
	roomRoof->draw();
	roomFloor->draw();
	glDisable(GL_CULL_FACE);
}

void drawRoom() {
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

	glDisable(GL_CULL_FACE);
}

void noShadowDisplay() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// projection transform
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1, 1, -1, 1, 1, 1000);

	// camera transform
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//light();
	
	gluLookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], 0, 1, 0);
	
	glPushMatrix();
		glTranslatef(live_object_xz_trans[0], live_object_y_trans, -live_object_xz_trans[1]);
		glMultMatrixf(live_object_rotation);
		
		initLight();
		drawScene();
		drawLight();

	glPopMatrix();
	glutSwapBuffers(); 
}

void setShadowMatrix(float fDestMat[16], float fLightPos[4], float fPlane[4])
{
    float dot;
    
    // dot product of plane and light position
    dot = fPlane[0] * fLightPos[0] + 
          fPlane[1] * fLightPos[1] + 
          fPlane[2] * fLightPos[2] + 
          fPlane[3] * 1.0;

     // first column
    fDestMat[0] = dot - fLightPos[0] * fPlane[0];
    fDestMat[4] = 0.0f - fLightPos[0] * fPlane[1];
    fDestMat[8] = 0.0f - fLightPos[0] * fPlane[2];
    fDestMat[12] = 0.0f - fLightPos[0] * fPlane[3];

    // second column
    fDestMat[1] = 0.0f - fLightPos[1] * fPlane[0];
    fDestMat[5] = dot - fLightPos[1] * fPlane[1];
    fDestMat[9] = 0.0f - fLightPos[1] * fPlane[2];
    fDestMat[13] = 0.0f - fLightPos[1] * fPlane[3];

    // third column
    fDestMat[2] = 0.0f - fLightPos[2] * fPlane[0];
    fDestMat[6] = 0.0f - fLightPos[2] * fPlane[1];
    fDestMat[10] = dot - fLightPos[2] * fPlane[2];
    fDestMat[14] = 0.0f - fLightPos[2] * fPlane[3];

    // fourth column
    fDestMat[3] = 0.0f - 1.0 * fPlane[0];
    fDestMat[7] = 0.0f - 1.0 * fPlane[1];
    fDestMat[11] = 0.0f - 1.0 * fPlane[2];
    fDestMat[15] = dot - 1.0 * fPlane[3];
}

void projectiveShadows(Quad *plane) {
	glClear(GL_STENCIL_BUFFER_BIT);
	// turning off writing to the color buffer and depth buffer so we only 
	// write to stencil buffer
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);

	// enable stencil buffer
	glEnable(GL_STENCIL_TEST);

	// write a one to the stencil buffer everywhere we are about to draw
	glStencilFunc(GL_ALWAYS, 1, 0xFFFFFFFF);

	// this is to always pass a one to the stencil buffer where we draw
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	// render the plane which the shadow will be on
	// color and depth buffer are disabled, only the stencil buffer
	// will be modified
	//draw();
	//roomFloor->drawWithTexture(texPlank);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	plane->draw();
	glDisable(GL_CULL_FACE);

	// turn the color and depth buffers back on
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	// until stencil test is diabled, only write to areas where the
	// stencil buffer has a one. This is to draw the shadow only on
	// the floor.
	glStencilFunc(GL_EQUAL, 1, 0xFFFFFFFF);

	// don't modify the contents of the stencil buffer
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

	// turn the light off
	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);

	// turn the depth test off
	glDisable(GL_DEPTH_TEST);

	// enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Multiply modelview matrix with shadow matrix
	float shadowMatrix[16];
	float planeEq[4];

	glPushMatrix();
		plane->getPlaneEq(planeEq);
		setShadowMatrix( shadowMatrix, lightPos, planeEq);
		glMultMatrixf(shadowMatrix);

		glColor4f(0.0,0.0,0.0, 0.5);

		drawRoomObjectsNoTex();

	glPopMatrix();

	// restore the state back to what it was
	glDisable(GL_BLEND);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glDisable(GL_STENCIL_TEST);
}

// projective shadows
void projectiveShadowDisplay() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

	// projection transform
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1, 1, -1, 1, 1, 1000);

	// camera transform
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], up[0], up[1], up[2]);
	
	glPushMatrix();
		glTranslatef(live_object_xz_trans[0], live_object_y_trans, -live_object_xz_trans[1]);
		glMultMatrixf(live_object_rotation);
		
		initLight();

		drawRoom();

		projectiveShadows(roomFloor);
		projectiveShadows(roomRoof);
		projectiveShadows(wallFront);
		projectiveShadows(wallBack);
		projectiveShadows(wallRight);
		projectiveShadows(wallLeft);
			
		drawLight();
		drawRoomObjects();
	glPopMatrix();
	glutSwapBuffers(); 
}

void shadowMapPass1() {
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT,(int*)viewport);

	glViewport(0,0,vpShadowMap,vpShadowMap);
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective ( 90, lightAspect, lightNearPlane, 200);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluLookAt(lightPos[0], lightPos[1], lightPos[2], lookat[0], lookat[1], lookat[2], up[0], up[1], up[2]);

	//Draw back faces into the shadow map
	glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    //Disable color writes, and use flat shading for speed
    glShadeModel(GL_FLAT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	initLight();
	drawRoomNoTex();
	if(enableCone) drawCone();
	//if(enableCubeMapping) cubeMapSphere();
	if(enableCubeMapping) cubeMapSphereNoTex();
	drawTori();

	//restore states
    glCullFace(GL_BACK);
    glShadeModel(GL_SMOOTH);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_CULL_FACE);

	glPopMatrix();

	//glGenTextures(1, &texShadowMap);
	glBindTexture(GL_TEXTURE_2D, texShadowMap);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, vpShadowMap, vpShadowMap, 0);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
}

void shadowMapPass2() {
	//wallBack->drawWithTexture(texShadowMap);
	
	GLfloat tmpMatrix1[16], tmpMatrix2[16];

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
		glLoadIdentity();
		glTranslatef(0.5,0.5,0.0);
		glScalef(0.5,0.5,1.0);
/*			static float biasMatrix[] = {0.5f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.5f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.5f, 0.0f,
								0.5f, 0.5f, 0.5f, 1.0f};
*/
		gluPerspective ( 90, lightAspect, 0.1, 100);
		gluLookAt(lightPos[0], lightPos[1], lightPos[2], lookat[0], lookat[1], lookat[2], up[0], up[1], up[2]);
//glMultMatrixf(biasMatrix);		
		glGetFloatv(GL_MODELVIEW_MATRIX, tmpMatrix1);

	glPopMatrix();

	transpose(tmpMatrix1, tmpMatrix2);

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

	glTexGenfv(GL_S, GL_EYE_PLANE, &tmpMatrix2[0]);
	glTexGenfv(GL_T, GL_EYE_PLANE, &tmpMatrix2[4]);
	glTexGenfv(GL_R, GL_EYE_PLANE, &tmpMatrix2[8]);
	glTexGenfv(GL_Q, GL_EYE_PLANE, &tmpMatrix2[12]);

	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);

	//Bind & enable shadow map texture
 	glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texShadowMap);

//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //Enable shadow comparison
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

    //Shadow comparison should be true (ie not in shadow) if r<=texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    //Shadow comparison should generate an INTENSITY result
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

     //Set alpha test to discard false comparisons
    glAlphaFunc(GL_GEQUAL, 0.99f);
    glEnable(GL_ALPHA_TEST);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glColor4f(1.0, 1.0, 1.0, 0.5);
	drawRoomSM();
    if(enableCone) drawCone();
	//if(enableCubeMapping) cubeMapSphere();
	if(enableCubeMapping) cubeMapSphere();
	drawTori();
	glDisable(GL_BLEND);

    //Disable textures and texgen
	glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);

    //Restore other states
    glDisable(GL_ALPHA_TEST);
}

// shadow mapping
void shadowMapDisplay() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

	shadowMapPass1();
	glClear(GL_DEPTH_BUFFER_BIT);

	// projection transform
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glFrustum(-1, 1, -1, 1, 1, 1000);
	gluPerspective(lightFOV, lightAspect, lightNearPlane, lightFarPlane);//(float)windowWidth/windowHeight, 1.0f, 100.0f);

	// camera transform
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	gluLookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], up[0], up[1], up[2]);
	
	glPushMatrix();
		glTranslatef(live_object_xz_trans[0], live_object_y_trans, -live_object_xz_trans[1]);
		glMultMatrixf(live_object_rotation);
		
		//Use dim light to represent shadowed areas
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
		//glLightfv(GL_LIGHT0, GL_AMBIENT, dimWhite);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dimWhite);
		glLightfv(GL_LIGHT0, GL_SPECULAR, black);
		lightPos[0] = liveLightMovement[0];
		lightPos[1] = PLIGHT_SPHERE_Y;
		lightPos[2] = -liveLightMovement[1];
		lightPos[3] = 0.0;
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
		glEnable(GL_LIGHT0);
		glEnable(GL_LIGHTING);	
		drawRoomSM();
		drawLight();
		glColor3f(1.0,1.0,0.0);
		drawTori();
		if(enableCubeMapping) cubeMapSphere();
		//if(enableCubeMapping) drawSphere();
		if(enableCone) drawCone();
		glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
		glLightfv(GL_LIGHT0, GL_SPECULAR, white);
		shadowMapPass2();

	glPopMatrix();
	glutSwapBuffers(); 


	/*glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

	shadowMapPass1();
	glClear(GL_DEPTH_BUFFER_BIT);	

	// projection transform
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1, 1, -1, 1, 1, 100);

	// camera transform
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], up[0], up[1], up[2]);
	
	glPushMatrix();
		glTranslatef(live_object_xz_trans[0], live_object_y_trans, -live_object_xz_trans[1]);
		glMultMatrixf(live_object_rotation);
		
		initLight();
		drawLight();
		drawRoomNoTex();
		drawRoomObjects();
		shadowMapPass2();
		
	glPopMatrix();
	glutSwapBuffers(); */
}

//
// SHADOW VOLUMES
//

struct surface *newSurface(float vertices[4][3])
{
	int i, j;
	struct surface *surf;

	surf = (struct surface *)malloc(sizeof(struct surface));

	for(i = 0; i < 4; i++) {
		for(j = 0; j < 3; j++)
			surf->vertices[i][j] = vertices[i][j];
	}
	return surf;
}

static void renderShadowVolume(struct surface *surf, float *surf_pos, float *light_pos) {
	int i,j, k;
	float v[4][3];
	float surfVert[16];
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glLoadIdentity();
	glTranslatef(surf_pos[0], surf_pos[1], surf_pos[2]);
	glMultMatrixf(liveCubeRotation);

	for(i = 0, k = 0; i<4;i++,k++) {
		for(j = 0; j<3; j++,k++)
			surfVert[k] = surf->vertices[i][j];
		surfVert[k] = 1.0;
	}
	glMultMatrixf(surfVert);
	glGetFloatv(GL_MODELVIEW_MATRIX, surfVert);
	glPopMatrix();

	float plane[4];
	//findEquation(&surfVert[0],&surfVert[4],&surfVert[8],plane);
	//if(isVertexAbovePlane(light_pos, plane) ){

	for(i = 0, k = 0; i <4; i++, k+=4) {
		v[i][0] = (surfVert[k] - light_pos[0]);
		v[i][1] = (surfVert[k+1] - light_pos[1]);
		v[i][2] = (surfVert[k+2] - light_pos[2]);
		normalize(v[i]);
		v[i][0] *= 25.0f;
		v[i][1] *= 25.0f;
		v[i][2] *= 25.0f;
	}

	// back cap
	glBegin(GL_QUADS);
		glVertex3fv(v[3]);
		glVertex3fv(v[2]);
		glVertex3fv(v[1]);
		glVertex3fv(v[0]);
	glEnd();

	// front cap
	glBegin(GL_QUADS);
		glVertex3fv(&surfVert[0]);
		glVertex3fv(&surfVert[4]);
		glVertex3fv(&surfVert[8]);
		glVertex3fv(&surfVert[12]);
	glEnd();

	glBegin(GL_QUAD_STRIP);
	glVertex3fv(&surfVert[0]);
	glVertex3fv(v[0]);
	for(i = 1, k = 4; i <= 4; i++, k+=4) {
		glVertex3fv(&(surfVert[(k%16)]));
		glVertex3fv(v[i % 4]);
	}
	glEnd();
	

}

struct cube *newCube(float size) {
	int i;
	struct cube *c;
	float v[4][3];
	int error = 0;

	c = (cube *)malloc(sizeof(struct cube));
	if(!c)
		return NULL;

	c->position[0] = 0.0f;
	c->position[1] = 0.0f;
	c->position[2] = 0.0f;

	v[0][0] = -size; v[0][1] = size; v[0][2] = size;
	v[1][0] = -size; v[1][1] = -size; v[1][2] = size;
	v[2][0] = size; v[2][1] = -size; v[2][2] = size;
	v[3][0] = size; v[3][1] = size; v[3][2] = size;
	c->surfaces[0] = newSurface(v);

	v[0][0] = size; v[0][1] = size; v[0][2] = -size;
	v[1][0] = size; v[1][1] = -size; v[1][2] = -size;
	v[2][0] = -size; v[2][1] = -size; v[2][2] = -size;
	v[3][0] = -size; v[3][1] = size; v[3][2] = -size;
	c->surfaces[1] = newSurface(v);

	v[0][0] = -size; v[0][1] = size; v[0][2] = -size;
	v[1][0] = -size; v[1][1] = size; v[1][2] = size;
	v[2][0] = size; v[2][1] = size; v[2][2] = size;
	v[3][0] = size; v[3][1] = size; v[3][2] = -size;
	c->surfaces[2] = newSurface(v);

	v[0][0] = size; v[0][1] = -size; v[0][2] = -size;
	v[1][0] = size; v[1][1] = -size; v[1][2] = size;
	v[2][0] = -size; v[2][1] = -size; v[2][2] = size;
	v[3][0] = -size; v[3][1] = -size; v[3][2] = -size;
	c->surfaces[3] = newSurface(v);

	v[0][0] = -size; v[0][1] = size; v[0][2] = size;
	v[1][0] = -size; v[1][1] = size; v[1][2] = -size;
	v[2][0] = -size; v[2][1] = -size; v[2][2] = -size;
	v[3][0] = -size; v[3][1] = -size; v[3][2] = size;
	c->surfaces[4] = newSurface(v);

	v[0][0] = size; v[0][1] = -size; v[0][2] = size;
	v[1][0] = size; v[1][1] = -size; v[1][2] = -size;
	v[2][0] = size; v[2][1] = size; v[2][2] = -size;
	v[3][0] = size; v[3][1] = size; v[3][2] = size;
	c->surfaces[5] = newSurface(v);

	for(i = 0; i < 6; i++) {
		if(!c->surfaces[i])
			error = 1;
	}

	if(error != 0) {
		for(i = 0; i < 6; i++) {
			if(c->surfaces[i])
				free(c->surfaces[i]);
		}

		free(c);
		return NULL;
	}

	return c;
}
void drawShadow() {
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(0, 1);
		glVertex2i(1, 1);
		glVertex2i(1, 0);
	glEnd();

	glDisable(GL_BLEND);

	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void renderSurface(struct surface *surf, float *position)
{
	glPushMatrix();
	glTranslatef(position[0], position[1], position[2]);

	glMultMatrixf(liveCubeRotation);

	// FIND NORMALS
	float n[3];
	// find 2 vectors lines
	float d1[3], d2[3];
	for(int i = 0; i < 3; i++) {
		d1[i] = surf->vertices[1][i] - surf->vertices[2][i];
		d2[i] = surf->vertices[2][i] - surf->vertices[3][i];
	}

	// find the normals
	crossproduct(d1, d2, n);
	normalize(n);

	glBegin(GL_QUADS);
		glNormal3fv(n);
		glVertex3fv(surf->vertices[0]);
		glNormal3fv(n);
		glVertex3fv(surf->vertices[1]);
		glNormal3fv(n);
		glVertex3fv(surf->vertices[2]);
		glNormal3fv(n);
		glVertex3fv(surf->vertices[3]);
	glEnd();

	glPopMatrix();
}

void renderCube(struct cube *c) {
	int i;

	for(i = 0; i < 6; i++)
		renderSurface(c->surfaces[i], c->position);
}

void renderCubeShadow(struct cube *c) {
	int i;

	for(i = 0; i < 6; i++) {
		glClear(GL_STENCIL_BUFFER_BIT);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);
		glEnable(GL_CULL_FACE);
		glEnable(GL_STENCIL_TEST);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0f, 100.0f);

		glCullFace(GL_FRONT);
		glStencilFunc(GL_ALWAYS, 0x0, 0xff);
		glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
		renderShadowVolume(c->surfaces[i], c->position, lightPos);

		glCullFace(GL_BACK);
		glStencilFunc(GL_ALWAYS, 0x0, 0xff);
		glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
		renderShadowVolume(c->surfaces[i], c->position, lightPos);

		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_CULL_FACE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);

		glStencilFunc(GL_NOTEQUAL, 0x0, 0xff);
		//glStencilFunc(GL_EQUAL, 0, ~0);
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_ZERO);
		drawShadow();
		glDisable(GL_STENCIL_TEST);
	}
}
// shadow volumes
void shadowVolumeDisplay() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

	// projection transform
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1, 1, -1, 1, 1, 1000);

	// camera transform
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], up[0], up[1], up[2]);
	
	glPushMatrix();
		glTranslatef(live_object_xz_trans[0], live_object_y_trans, -live_object_xz_trans[1]);
		glMultMatrixf(live_object_rotation);
			
		initLight();
		drawLight();

		drawScene();
	
		static struct cube *occluder;
		occluder = newCube(OCCLUDER_SIZE);
		occluder->position[0] = occluderPosition[0]+liveCubeXZTrans[0];
		occluder->position[1] = occluderPosition[1]+liveCubeYTrans;
		occluder->position[2] = occluderPosition[2]+liveCubeXZTrans[1];

		renderCube(occluder);
		renderCubeShadow(occluder);

	glPopMatrix();
	glutSwapBuffers(); 
}

// draw the scene
void myGlutDisplay(	void )
{
	switch(liveShadowType) {
		case NO_SHADOWS:
			noShadowDisplay();
			break;
		case SHADOW_PROJECTIVE:
			projectiveShadowDisplay();
			break;
		case SHADOW_MAPPING:
			shadowMapDisplay();
			break;
		case SHADOW_VOLUMES:
			shadowVolumeDisplay();
			break;
			
	}
	
}

void initCamera() {

	initEye[0] = eye[0] = 0;
	initEye[1] = eye[1] = 5;
	initEye[2] = eye[2] = 15;
	initLookat[0] = lookat[0] = 0;
	initLookat[1] = lookat[1] = 4;
	initLookat[2] = lookat[2] = 0;
	up[0] = 0; up[1] = 1; up[2] = 0;
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
	lightDif = 1.0;//, lightDifG = 1.0, lightDifB = 1.0;
	//lightSpecR = 1.0, lightSpecG = 1.0, lightSpecB = 1.0;
}

void resetLightSpinners() {
	//spin_s1->set_float_val(lightAmbR);
	//spin_s2->set_float_val(lightAmbG);
	//spin_s3->set_float_val(lightAmbB);
	spin_s4->set_float_val(lightDif);
	//spin_s5->set_float_val(lightDifG);
	//spin_s6->set_float_val(lightDifB);
	//spin_s7->set_float_val(lightSpecR);
	//spin_s8->set_float_val(lightSpecG);
	//spin_s9->set_float_val(lightSpecB);
}

// initialize textures
void initTextures() {
	static unsigned *image;
	static int width, height, components;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// initializing the plank
	glGenTextures(1, &texPlank);
	glBindTexture(GL_TEXTURE_2D, texPlank);
    
	image = read_texture(TEX_IMAGE_PLANK, &width, &height, &components);

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
    
	image = read_texture(TEX_IMAGE_EARTH, &width, &height, &components);

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
    
	image = read_texture(TEX_IMAGE_LIGHTMAP, &width, &height, &components);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, components, width,
		height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		image);

	free(image);

	glGenTextures(1, &texShadowMap);
    glBindTexture(GL_TEXTURE_2D, texShadowMap);
    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, vpShadowMap, vpShadowMap, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
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

	case CB_RESET_LIGHT_POS:
		liveLightMovement[0] = 0;
		liveLightMovement[1] = -PLIGHT_SPHERE_Z;
		liveLightYMovement = PLIGHT_SPHERE_Y;
		lightTranslation->set_x(0);
		lightTranslation->set_y(-PLIGHT_SPHERE_Z);
		lightYTranslation->set_y(PLIGHT_SPHERE_Y);

		break;
	}

	glutPostRedisplay();
}

// entry point
int main(int argc,	char* argv[])
{
  if(argc != 4) {
    cerr << "Usage: <program> <texture1_plank> <texture2_earth> <texture3_lightmap>" << endl;
    return -1;
  }
  else {
    TEX_IMAGE_PLANK = argv[1];
    TEX_IMAGE_EARTH = argv[2];
    TEX_IMAGE_LIGHTMAP = argv[3];
  }

  //
  // create the glut window
  //
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH|GLUT_STENCIL);
  glutInitWindowSize(windowWidth, windowHeight);
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
  liveLightMovement[0] = 0;
  liveLightMovement[1] = -PLIGHT_SPHERE_Z;
  liveLightYMovement = PLIGHT_SPHERE_Y;
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

  // the object rollout
  object_rollout = glui->add_rollout("Shadow Types");
	
  // the radio buttons
  object_type_radio = glui->add_radiogroup_to_panel(object_rollout, &liveShadowType);
  glui->add_radiobutton_to_group(object_type_radio, "No Shadow");
  glui->add_radiobutton_to_group(object_type_radio, "Projective Shadows");
  glui->add_radiobutton_to_group(object_type_radio, "Shadow Mapping");
  glui->add_radiobutton_to_group(object_type_radio, "Shadow Volumes");
  // Rotation and translation controls
  transform_panel = glui->add_panel_to_panel(object_rollout, "",GLUI_PANEL_NONE);
  cubeRotation = glui->add_rotation_to_panel(transform_panel, "Rotation", liveCubeRotation);
  cubeRotation->reset();
  glui->add_column_to_panel(transform_panel, false);
  cubeXZTrans = glui->add_translation_to_panel(transform_panel, "Cube XZ", GLUI_TRANSLATION_XY, liveCubeXZTrans);
  glui->add_column_to_panel(transform_panel, false);
  cubeYTrans = glui->add_translation_to_panel(transform_panel, "Cube Y", GLUI_TRANSLATION_Y, &liveCubeYTrans);
  cubeXZTrans->scale_factor = 0.1f;
  cubeYTrans->scale_factor = 0.1f;
  // empty space
  glui->add_statictext("");

  // the walk control
  anim_rollout = glui->add_rollout("Light");
  spin_s4 = glui->add_spinner_to_panel(anim_rollout, "Intensity", GLUI_SPINNER_FLOAT, &lightDif);
  spin_s4->set_float_limits(0.0, 1.0);
  action_button = glui->add_button_to_panel(anim_rollout, "Reset Light Intensity", CB_RESET_LIGHT_BUTTON, glui_cb);

  transform_panel = glui->add_panel_to_panel(anim_rollout, "", GLUI_PANEL_NONE);
  lightTranslation = glui->add_translation_to_panel(transform_panel, "Translate Light XY", 
						    GLUI_TRANSLATION_XY, liveLightMovement);
  glui->add_column_to_panel(transform_panel, false);
  lightYTranslation = glui->add_translation_to_panel(transform_panel, "Translate Light Y", 
						     GLUI_TRANSLATION_Y, &liveLightYMovement);
  lightTranslation->scale_factor = 0.05f;
  lightYTranslation->scale_factor = 0.05f;
  action_button = glui->add_button_to_panel(anim_rollout, "Reset Light", CB_RESET_LIGHT_POS, glui_cb);
	
  // the object rollout
  object_rollout = glui->add_rollout("Cone texturing");
	
  glui->add_checkbox_to_panel(object_rollout, "Draw Cone", &enableCone);
  // the radio buttons
  object_type_radio = glui->add_radiogroup_to_panel(object_rollout, &coneTexturingType);
  glui->add_radiobutton_to_group(object_type_radio, "Decal");
  glui->add_radiobutton_to_group(object_type_radio, "Modulate");
  glui->add_radiobutton_to_group(object_type_radio, "Replace");
  glui->add_radiobutton_to_group(object_type_radio, "Blend");

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
  initCamera();

  // initialize gl
  //glEnable(GL_BLEND);
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_COLOR_MATERIAL);
  initTextures();

  // initialize glu
  quadric  = gluNewQuadric();
  gluQuadricTexture(quadric, GL_TRUE);
  gluQuadricDrawStyle(quadric, GLU_FLAT);

  // give control over to glut
  glutMainLoop();

  return 0;
}
