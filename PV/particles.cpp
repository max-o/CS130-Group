//
//  main.cpp
//  sphere
//
//  Created by Scott Friedman on 10/10/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include <fstream>
#include "Angel.h"
#ifdef __APPLE__  // include Mac OS X verions of headers
#  include <OpenGL/OpenGL.h>
#  include <GLUT/glut.h>
#else // non-Mac OS X operating systems
#  include <GL/glew.h>
#  include <GL/freeglut.h>
#  include <GL/freeglut_ext.h>
#endif  // __APPLE__


void init( void );
void display( void );
void keyboard( unsigned char key, int x, int y );
void reshape( int width, int height );

//----
#include <iostream>
#include <math.h>
typedef vec4 point4;
typedef vec4 color4;

const int NumTimesToSubdivide = 5; // 6, 5, 4, 3, 2, 1, 0
// (4 faces) ^ (NumTimesToSubdivide + 1)
const int NumTriangles =  4096; // 16384, 4096, 1024, 256, 64, 16, 4

const int NumVertices = 3 * NumTriangles;

point4 points[NumVertices * 5];
vec3   normals[NumVertices * 5];

GLuint ModelView, Projection, shadingType;

int Index = 0;

void triangle( const point4& a, const point4& b, const point4& c );
point4 unit( const point4& p );
void divide_triangle( const point4& a, const point4& b, const point4& c, int count );
void tetrahedron( int count );

void triangle( const point4& a, const point4& b, const point4& c )
{
	vec3 normal = normalize(cross(b-a,c-b));
	if (Index >=  2 * NumVertices || Index >= NumVertices) // Gourad or Phong shading, average normals (planet 2 & 3)
	{ // such a small scale that normals are just vertices (normalized)
		vec3 x(a.x, a.y, a.z);
		vec3 y(a.x, a.y, a.z);
		vec3 z(a.x, a.y, a.z);

		normals[Index] = normalize(x); points[Index] = a; Index++;
		normals[Index] = normalize(y); points[Index] = b; Index++;
		normals[Index] = normalize(z); points[Index] = c; Index++;
	}
	else // flat shading, don't average normals
	{
		normals[Index] = normal; points[Index] = a; Index++;
		normals[Index] = normal; points[Index] = b; Index++;
		normals[Index] = normal; points[Index] = c; Index++;
	}
}

point4 unit( const point4& p )
{
    float len = p.x * p.x + p.y * p.y + p.z * p.z;
    
    point4 t;
    if ( len > DivideByZeroTolerance )
    {
        t = p / sqrt(len);
        t.w = 1.0;
    }
    return t;
}

void divide_triangle( const point4& a, const point4& b, const point4& c, int count )
{
    if ( count > 0 )
    {
        point4 v1 = unit( a + b );
        point4 v2 = unit( a + c );
        point4 v3 = unit( b + c );
        divide_triangle( a, v1, v2, count - 1);
        divide_triangle( c, v2, v3, count - 1);
        divide_triangle( b, v3, v1, count - 1);
        divide_triangle(v1, v3, v2, count - 1);
    }
    else
    {
		triangle(a, b, c );
    }
}

void tetrahedron( int count )
{
    point4 v[4] = 
    {
        vec4( 0.0f, 0.0f, 1.0f, 1.0f ),
        vec4( 0.0f, 0.942809f, -0.333333f, 1.0f ),
        vec4( -0.816497f, -0.471405f, -0.333333f, 1.0f ),
        vec4( 0.816497f, -0.471405f, -0.333333f, 1.0f ),
    };
    divide_triangle(v[0], v[1], v[2], count);
    divide_triangle(v[3], v[2], v[1], count);
    divide_triangle(v[0], v[3], v[1], count);
    divide_triangle(v[0], v[2], v[3], count);
}


//----
GLuint program;
void init( void )
{
    // subdivide a tetrahedron into a sphere


  
	tetrahedron( NumTimesToSubdivide );
	tetrahedron( NumTimesToSubdivide );
	tetrahedron( NumTimesToSubdivide );
	tetrahedron( NumTimesToSubdivide );
	tetrahedron( NumTimesToSubdivide );
 
    GLuint vao;
#ifdef __APPLE__  // include Mac OS X verions of headers
    glGenVertexArraysAPPLE(1, &vao);
    glBindVertexArrayAPPLE(vao);
#else
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#endif

    // create and initialize a buffer object
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points)+sizeof(normals), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(normals), normals);
    
    // load shaders and use the resulting shader programs
    program = InitShader("vShader.glsl", "fShader.glsl");
    glUseProgram(program);

    // setup vertex arrays
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(vPosition);
    GLuint vNormal = glGetAttribLocation(program, "vNormal");
    glEnableVertexAttribArray(vNormal);
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points)));
    glClearColor( 0.5, 0.5, 0.5, 1.0 ); 
    
    
    // retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");
    
    glEnable( GL_DEPTH_TEST );
    
 
}

point4  position(-10, 8, 15, 1.0);
point4  at( 0.0, 0.0, 0.0, 1.0 );
vec4    up( 0.0, 1.0, 0.0, 1.0 );
vec4    direction = at - position;
float p1x = 0; float p1z = 0;
float p2x = 0; float p2z = 0;
float p3x = 0; float p3z = 0;
float p4x = 0; float p4z = 0;
float py1 = 0; float py2 = 0; 



void display( void )					


{
	float shadeType = 0; 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	
	point4 light_position(position.x, position.y, position.z, 1);	// light is at camera position, always lighting the sun
	light_position = LookAt(position, position + direction, up) * light_position;

	shadeType = 0.f; // Phong shading
    color4 light_ambient(   1.f, 0.5f, 1.f, 1.0f );
    color4 light_diffuse(   1.f, 1.f, 1.f, 1.0f );
    color4 light_specular(  0.f, 0.f, 0.f, 1.0f );
    
    color4 material_ambient( 0.1f, 0.0f, 0.0f, 1.0f );
    color4 material_diffuse( 0.5f, 0.0f, 0.0f, 1.0f );
    color4 material_specular( .1f, 0.1f, 0.1f, 1.0f );
    float  material_shininess = 50.f;

    
    color4 ambient_product = light_ambient * material_ambient;
    color4 diffuse_product = light_diffuse * material_diffuse;
    color4 specular_product = light_specular * material_specular;
    
    glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, ambient_product);
    glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, diffuse_product);
    glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, specular_product);
    glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1, light_position);
    glUniform1f(glGetUniformLocation(program, "Shininess"), material_shininess);
	glUniform1f(glGetUniformLocation(program, "shadingType"), shadeType);
	
	std::ifstream file;
	file.open("positions.txt");
	mat4 model_view;
	if (!file.is_open() )
	{
		return;
	}

	int x, y, z;
	double mass;
	int count = 0;
	while (!file.eof())
	{
		file >> x >> y >> z >> mass;
		if (count % 2 == 0)
			model_view = LookAt(position, position + direction, up) * Translate(x, py1+y, z) * Scale(mass, mass, mass);
		else
			model_view = LookAt(position, position + direction, up) * Translate(x, py2+y, z) * Scale(mass, mass, mass);
		glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
		glDrawArrays(GL_TRIANGLES, 4 * NumVertices, NumVertices);
		count++;
	}
	
	
	glutSwapBuffers();
}

GLfloat zNear = 1.0f; GLfloat zFar = 30.0f;
void reshape( int width, int height )
{
    glViewport(0, 0, width, height);
    
    GLfloat left = -2.0f, right = 2.0f;
    GLfloat top = 2.0f, bottom = -2.0f;
    zNear = 1.0f; zFar = 30.0f;
    
    GLfloat aspect = GLfloat(width)/height;
	
    
    if ( aspect > 1.0f )
    {
        left *= aspect;
        right *= aspect;
    }
    else
    {
        top /= aspect;
        bottom /= aspect;
    }
    
    //mat4 projection = Ortho(left, right, bottom, top, zNear, zFar);
    mat4 projection = Perspective(45.0f, aspect, zNear, zFar);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
}
float k = 0;

const float MIN_DIST_INCREMENT = 0.25;
float speed = MIN_DIST_INCREMENT;
const float MAX_DIST_INCREMENT = 10.0;
const float SPEED_INCREMENT = 0.5;

void keyboard( unsigned char key, int x, int y )
{
    switch (key) {
        case 033:
        case 'q': case 'Q':
            exit(EXIT_SUCCESS);
            break;
		case 'a': // move left 0.25 units
			position = position + MIN_DIST_INCREMENT * normalize(cross(up, direction));		
			break;
		case 'd': // move right 0.25 units
			position = position - MIN_DIST_INCREMENT * normalize(cross(up, direction));	
			break;		
		case 'w': // camera 'moves' upward
			position.y += MIN_DIST_INCREMENT;		
			break;		
		case 's': // camera 'moves' downward
			position.y -= MIN_DIST_INCREMENT;		
			break;  
		case ' ': // reset
			position  = point4(-10, 8, 15, 1.0);
			at        = point4( 0.0, 0.0, 0.0, 1.0 );
			up        = point4( 0.0, 1.0, 0.0, 0.0 );
            direction = at - position;			
			zNear     = 1.0f;
			zFar	  = 30.0f;
			break;
		case 'u': // pivot upward
			direction = RotateX(1.0) * direction;			
			break;
		case 'j': // pivot upward
			direction = RotateX(-1.0) * direction;
			break;
	   default:
            break;

		glutPostRedisplay();
    }
}

void SpecialKeys(int key, int x, int y) 
// special function for arrow keys
{
    switch (key)
	{
		case GLUT_KEY_LEFT:  // camera 'heads' left one degree
			direction = RotateY(1.0) * direction;			
			break;
		case GLUT_KEY_RIGHT: // camera 'heads' right one degree
			direction = RotateY(-1.0) * direction;			
			break;		
		case GLUT_KEY_DOWN: // move backward 0.25 units
			position = position - speed * normalize(direction);	
			if (speed < MAX_DIST_INCREMENT)
				speed += SPEED_INCREMENT;
			break;      
		case GLUT_KEY_UP:   // move forward 0.25 units
			position = position + speed * normalize(direction);	
			if (speed < MAX_DIST_INCREMENT)
				speed += SPEED_INCREMENT;
			break;  
	}
	glutPostRedisplay();
}

void Release(int key, int x, int y) 
// special function to reset speed if user releases key
{
    switch (key)
	{
	case GLUT_KEY_UP:		
	case GLUT_KEY_DOWN:		
		speed = MIN_DIST_INCREMENT;
		break;
	}

	glutPostRedisplay();
}


float p1time = .04;
float p2time = .04;
float p3time = 120;
float p4time = 300;
bool descent1 = false;
bool descent2 = true;
void idle()
{
	if (descent1 == false)
	{
		py1 += p1time/2;
		p1time += 0.002;
		if (p1time > .05)
		{
			descent1 = true;
		}
	}
	else if (descent1 == true)
	{
		py1 -= p1time/2;
		p1time -= 0.002;
		if (p1time < .03)
		{
			descent1 = false;
		}
	}

	if (descent2 == false)
	{
		py2 += p2time/2;
		p2time += 0.001;
		if (p2time > .05)
		{
			descent2 = true;
		}
	}
	else if (descent2 == true)
	{
		py2 -= p2time/2;
		p2time -= 0.001;
		if (p2time < .03)
		{
			descent2 = false;
		}
	}
	
	

	glutPostRedisplay();
	
}

int main (int argc, char *argv[] )
{
     
    glutInit( &argc, argv );
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 50);
	glutInitWindowSize(800, 600);
    glutCreateWindow( "Sphere" );
    
#ifndef __APPLE__  // include Mac OS X verions of headers
	glewInit();
#endif
	
	init();
    
    glutDisplayFunc(display);
	glutIdleFunc(idle);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
	glutSpecialFunc(SpecialKeys); // special function for arrow keys
	glutSpecialUpFunc(Release);
    
    glutMainLoop();
    return 0;
}

