#include <GL/glew.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <math.h>

#define PI 3.14159265

class Cone {
public: 
	Cone() {}
	Cone(float radius, float height, float numOfQuads) {
		this->radius = radius;
		this->height = height;
		this->numOfQuads = numOfQuads;
		this->theta = PI*2/numOfQuads;
	}

	void drawWithTexture(GLuint texName, int sRepeat, int tRepeat, float texEnvMode) {
		float vert0[3] = {0.0, 0.0, 0.0}; vert0[1] = height;
		float vert1[3] = {0.0, 0.0, 0.0}; vert1[0] = radius;
		float vert2[3] = {0.0, 0.0, 0.0};
		float d1[3], d2[3], n[3];
		float t1[2], t2[2], t3[2], t4[2];

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texEnvMode);
		glBindTexture(GL_TEXTURE_2D, texName);

		glColor4f(1.0, 1.0, 1.0, 1.0);

		GLfloat matEmission[] = {0.3,0.3,0.3,0.0};
		GLfloat matDefaultEmission[] = {0.0, 0.0, 0.0, 1.0};
		glMaterialfv(GL_FRONT, GL_EMISSION, matEmission);
		glBegin(GL_QUADS);
			for(int i = 0; i < numOfQuads; i++) {
				vert2[0] = radius * cos(theta*(i+1));
				vert2[2] = radius * sin(theta*(i+1));

				t1[0] = sRepeat*(numOfQuads-i+1)/numOfQuads;
				t1[1] = 0;

				t2[0] = sRepeat*(numOfQuads-i)/numOfQuads;
				t2[1] = 0;

				t3[0] = t2[0];
				t3[1] = tRepeat;

				t4[0] = t1[0];
				t4[1] = tRepeat;

				for(int j = 0; j < 3; j++) {
					d1[j] = vert2[j]-vert0[j];
					d2[j] = vert1[j]-vert0[j];
				}
				// find the normals
				this->crossProduct(d1, d2, n);

				glTexCoord2fv(t1);
				glNormal3fv(n);
				glVertex3fv(vert2);
				glTexCoord2fv(t2);
				glNormal3fv(n);
				glVertex3fv(vert1);
				glTexCoord2fv(t3);
				glNormal3fv(n);
				glVertex3fv(vert0);
				glTexCoord2fv(t4);
				glNormal3fv(n);
				glVertex3fv(vert0);	

				vert1[0] = vert2[0];
				vert1[2] = vert2[2];
			}
		glEnd();
		glFlush();
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glMaterialfv(GL_FRONT, GL_EMISSION, matDefaultEmission);
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
	float radius, height, numOfQuads, theta;
};
