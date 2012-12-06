/* particles.h */
#ifndef _PARTICLES_H
#define _PARTICLES_H

#include <iostream>
#include <fstream>
#include "Angel.h"
#include <math.h>
#ifdef __APPLE__  // include Mac OS X verions of headers
#  include <OpenGL/OpenGL.h>
#  include <GLUT/glut.h>
#else // non-Mac OS X operating systems
#  include <GL/glew.h>
#  include <GL/freeglut.h>
#  include <GL/freeglut_ext.h>
#endif  // __APPLE__

typedef vec4 color4;
typedef vec4 point4;
const int NUM_TIMES_TO_SUBDIVIDE = 5; // 6, 5, 4, 3, 2, 1, 0
// (4 faces) ^ (NUM_TIMES_TO_SUBDIVIDE + 1)
const int NUM_TRIANGLES =  4096; // 16384, 4096, 1024, 256, 64, 16, 4
const int NUM_VERTICES = 3 * NUM_TRIANGLES;
const float MIN_DIST_INCREMENT = 0.25;
const float MAX_DIST_INCREMENT = 10.0;
const float SPEED_INCREMENT = 0.5;


class Visualizer
{
public:
	Visualizer();


	void triangle( const point4& a, const point4& b, const point4& c );
	point4 unit( const point4& p );
	void divide_triangle( const point4& a, const point4& b, const point4& c, int count );
	void tetrahedron( int count );
	void init( void );
	static void display( void );
	static void reshape( int width, int height );
	static void keyboard( unsigned char key, int x, int y );
	static void SpecialKeys(int key, int x, int y);
	static void Release(int key, int x, int y);
	static void idle();
	void visualize();
	static int getConnection();
	static void mainReadLoop(int connectFD);
	static void registerWithSimulation();//boost::thread visualizerThread;

private:
	   
	static GLuint program;
	static point4 points[NUM_VERTICES * 5];
	static vec3   normals[NUM_VERTICES * 5];
	static int Index;
	static GLuint ModelView;
	static GLuint Projection;
	static GLuint shadingType;
	static point4  position;
	static point4  at; 
	static vec4    up; 
	static vec4    direction;
	static float speed;
	static GLfloat zNear;
	static GLfloat zFar;

	// Ring buffer
	static float *positions;
	static float *readPos;
	static float *writePos;
	static int numParticles;
	static int bufferSize;
	static int floatsPerBuffer;
	static bool ready;

	// TO BE DELETED
	static float p1x; static float p1z;
	static float p2x; static float p2z;
	static float p3x; static float p3z;
	static float p4x; static float p4z;
	static float py1; static float py2; 
	static float k;
	static float p1time; static float p2time;
	static float p3time; static float p4time;
	static bool descent1; static bool descent2;
	// END TO BE DELETED

};


#endif

