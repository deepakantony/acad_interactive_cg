#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif
#include <math.h>

class Quad {
public: 
	Quad() {}
	Quad(float v1[3], float v2[3], float v3[3], float v4[3]) {
		this->v1[0] = v1[0]; this->v1[1] = v1[1];this->v1[2] = v1[2];
		this->v2[0] = v2[0]; this->v2[1] = v2[1];this->v2[2] = v2[2];
		this->v3[0] = v3[0]; this->v3[1] = v3[1];this->v3[2] = v3[2];
		this->v4[0] = v4[0]; this->v4[1] = v4[1];this->v4[2] = v4[2];

		// find 2 vectors lines
		float d1[3], d2[3];
		for(int i = 0; i < 3; i++) {
			d1[i] = v1[i] - v2[i];
			d2[i] = v2[i] - v3[i];
		}

		// find the normals
		this->crossProduct(d1, d2, n);
	}

	void initTexture(float t1[2], float t2[2], float t3[2], float t4[2]) {
		this->t1[0] = t1[0]; this->t1[1] = t1[1];
		this->t2[0] = t2[0]; this->t2[1] = t2[1];
		this->t3[0] = t3[0]; this->t3[1] = t3[1];
		this->t4[0] = t4[0]; this->t4[1] = t4[1];
	}

	void draw() {
		glDisable(GL_COLOR_MATERIAL);
		glBegin(GL_QUADS);
			glNormal3fv(this->n);
			glVertex3fv(this->v1);
			glNormal3fv(this->n);
			glVertex3fv(this->v2);
			glNormal3fv(this->n);
			glVertex3fv(this->v3);
			glNormal3fv(this->n);
			glVertex3fv(this->v4);
		glEnd();
		glEnable(GL_COLOR_MATERIAL);
	}

	void drawWithTexture(GLuint texName) {
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glBindTexture(GL_TEXTURE_2D, texName);
		glBegin(GL_QUADS);
			glNormal3fv(this->n);
			glTexCoord2fv(t1);
			glVertex3fv(this->v1);
			glNormal3fv(this->n);
			glTexCoord2fv(t2);
			glVertex3fv(this->v2);
			glNormal3fv(this->n);
			glTexCoord2fv(t3);
			glVertex3fv(this->v3);
			glNormal3fv(this->n);
			glTexCoord2fv(t4);
			glVertex3fv(this->v4);
		glEnd();
		glFlush();
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	void drawWithMultiTexture(GLuint texName1, GLuint texName2, float translate[2]) {
		// Initilize 2 textures for light mapping
		glEnable(GL_COLOR_MATERIAL);
		glColor3f(1.0,1.0,1.0);
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texName1);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texName2);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			glLoadIdentity();
			glTranslatef(translate[1], translate[0], 0.0);
		glMatrixMode(GL_MODELVIEW);
		glBegin(GL_QUADS);
			glNormal3fv(this->n);
			glMultiTexCoord2fv(GL_TEXTURE0, t1);
			glMultiTexCoord2f(GL_TEXTURE1, 0, 0);
			glVertex3fv(this->v1);
			glNormal3fv(this->n);
			glMultiTexCoord2fv(GL_TEXTURE0, t2);
			glMultiTexCoord2f(GL_TEXTURE1, 0, 1);
			glVertex3fv(this->v2);
			glNormal3fv(this->n);
			glMultiTexCoord2fv(GL_TEXTURE0, t3);
			glMultiTexCoord2f(GL_TEXTURE1, 1, 1);
			glVertex3fv(this->v3);
			glNormal3fv(this->n);
			glMultiTexCoord2fv(GL_TEXTURE0, t4);
			glMultiTexCoord2f(GL_TEXTURE1, 1, 0);
			glVertex3fv(this->v4);
		glEnd();
		glFlush();
		//glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glMatrixMode(GL_TEXTURE);
			glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		//glEnable(GL_COLOR_MATERIAL);
	}

private:
	void normalize(float v[3]) {
		GLfloat d = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
		v[0] /= d;
		v[1] /= d;
		v[2] /= d;
	}

	void crossProduct(float v1[3], float v2[3], float out[3]) {
		out[0] = v1[1]*v2[2]-v1[2]*v2[1];
		out[1] = v1[2]*v2[0]-v1[0]*v2[2];
		out[2] = v1[0]*v2[1]-v1[1]*v2[0];

		this->normalize(out);
	}

private:
	// vertices
	float v1[3], v2[3], v3[3], v4[3];
	// normal to the quad
	float n[3];
	// texture coordinates
	float t1[2], t2[2], t3[2], t4[2];
};
