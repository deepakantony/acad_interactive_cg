#include "Quad.cpp"

class Cube {
public:
	Cube() {};
	Cube( float side, const float position[3]) {
		int x = 1, y = 1, z = 1;

		/* cube is as follows
		 		  v4*********v0
					*		*
		 	  v5********v1	*
				*	*  *	*
				*	*  *	*
				* v6*********v2
				*	   *
			  v7********v3
		 */
		for(int i = 1; i<=8; i++) {
			this->v[i-1][0] = position[0]+x*side/2/2;
			this->v[i-1][1] = position[1]+y*side/2/2;
			this->v[i-1][2] = position[2]+z*side/2/2;
			if(i%4 == 0) x = -x;
			if(i%2 == 0) y = -y;
			z = -z;
		}

		this->q[0] = new Quad(v[3], v[7], v[5], v[1]); // back
		this->q[1] = new Quad(v[2], v[0], v[4], v[6]); // front
		//this->q[1] = new Quad(v[6], v[4], v[0], v[2]); // front
		//this->q[2] = new Quad(v[1], v[0], v[4], v[5]); // top
		this->q[2] = new Quad(v[5], v[4], v[0], v[1]); // top
		//this->q[3] = new Quad(v[3], v[7], v[6], v[2]); // bottom
		this->q[3] = new Quad(v[2], v[6], v[7], v[3]); // bottom
		//this->q[4] = new Quad(v[0], v[1], v[3], v[2]); // right
		this->q[4] = new Quad(v[2], v[3], v[1], v[0]); // right
		//this->q[5] = new Quad(v[6], v[7], v[5], v[4]); // left
		this->q[5] = new Quad(v[4], v[5], v[7], v[6]); // left 
		x = y;

	/*	float v[4][3];

		v[0][0] = -side/2; v[0][1] = side/2; v[0][2] = side/2;
		v[1][0] = -side/2; v[1][1] = -side/2; v[1][2] = side/2;
		v[2][0] = side/2; v[2][1] = -side/2; v[2][2] = side/2;
		v[3][0] = side/2; v[3][1] = side/2; v[3][2] = side/2;
		this->q[0] = new Quad(v[0], v[1], v[2], v[3]);
		//c->surfaces[0] = new_surface(v);

		v[0][0] = side/2; v[0][1] = side/2; v[0][2] = -side/2;
		v[1][0] = side/2; v[1][1] = -side/2; v[1][2] = -side/2;
		v[2][0] = -side/2; v[2][1] = -side/2; v[2][2] = -side/2;
		v[3][0] = -side/2; v[3][1] = side/2; v[3][2] = -side/2;
		//c->surfaces[1] = new_surface(v);
		this->q[1] = new Quad(v[0], v[1], v[2], v[3]);

		v[0][0] = -side/2; v[0][1] = side/2; v[0][2] = -side/2;
		v[1][0] = -side/2; v[1][1] = side/2; v[1][2] = side/2;
		v[2][0] = side/2; v[2][1] = side/2; v[2][2] = side/2;
		v[3][0] = side/2; v[3][1] = side/2; v[3][2] = -side/2;
		//c->surfaces[2] = new_surface(v);
		this->q[2] = new Quad(v[0], v[1], v[2], v[3]);

		v[0][0] = side/2; v[0][1] = -side/2; v[0][2] = -side/2;
		v[1][0] = side/2; v[1][1] = -side/2; v[1][2] = side/2;
		v[2][0] = -side/2; v[2][1] = -side/2; v[2][2] = side/2;
		v[3][0] = -side/2; v[3][1] = -side/2; v[3][2] = -side/2;
		//c->surfaces[3] = new_surface(v);
		this->q[3] = new Quad(v[0], v[1], v[2], v[3]);

		v[0][0] = -side/2; v[0][1] = side/2; v[0][2] = side/2;
		v[1][0] = -side/2; v[1][1] = side/2; v[1][2] = -side/2;
		v[2][0] = -side/2; v[2][1] = -side/2; v[2][2] = -side/2;
		v[3][0] = -side/2; v[3][1] = -side/2; v[3][2] = side/2;
		//c->surfaces[4] = new_surface(v);
		this->q[4] = new Quad(v[0], v[1], v[2], v[3]);

		v[0][0] = side/2; v[0][1] = -side/2; v[0][2] = side/2;
		v[1][0] = side/2; v[1][1] = -side/2; v[1][2] = -side/2;
		v[2][0] = side/2; v[2][1] = side/2; v[2][2] = -side/2;
		v[3][0] = side/2; v[3][1] = side/2; v[3][2] = side/2;
		//c->surfaces[5] = new_surface(v);
		this->q[5] = new Quad(v[0], v[1], v[2], v[3]);*/
	}

	void draw() {
		for(int i = 0; i < 6; i++)
			q[i]->drawWithoutNormals();
	}

	Quad* getQuad(int i) {
		return q[i];
	}

private:
	Quad* q[6];
	float v[8][3];
};