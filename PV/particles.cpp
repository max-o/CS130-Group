//
//  particles.cpp
//

#include "Angel.h"
#include "particles.h"

#include <iostream>
#include <fstream>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __APPLE__  // include Mac OS X verions of headers
#  include <OpenGL/OpenGL.h>
#  include <GLUT/glut.h>
#else // non-Mac OS X operating systems
#  include <GL/glew.h>
#  include <GL/freeglut.h>
#  include <GL/freeglut_ext.h>
#endif  // __APPLE__

using namespace std;

vec3   Visualizer::normals[NUM_VERTICES * 5];
GLuint Visualizer::ModelView;
GLuint Visualizer::Projection;
GLuint Visualizer::shadingType;

GLuint Visualizer::program;
point4 Visualizer::points[NUM_VERTICES * 5];
int Visualizer::Index = 0;

point4 Visualizer::position(-10, 8, 15, 1.0);
point4 Visualizer::at( 0.0, 0.0, 0.0, 1.0 );
vec4   Visualizer::up( 0.0, 1.0, 0.0, 1.0 );
vec4   Visualizer::direction = at - position;
float  Visualizer::speed = MIN_DIST_INCREMENT;

GLfloat Visualizer::zNear = 1.0f; 
GLfloat Visualizer::zFar = 30.0f;

// TO BE DELETED
float Visualizer::p1x = 0;
float Visualizer::p1z = 0;
float Visualizer::p2x = 0; 
float Visualizer::p2z = 0;
float Visualizer::p3x = 0; 
float Visualizer::p3z = 0;
float Visualizer::p4x = 0; 
float Visualizer::p4z = 0;
float Visualizer::py1 = 0; 
float Visualizer::py2 = 0; 
float Visualizer::k = 0;
float Visualizer::p1time = .04f;
float Visualizer::p2time = .04f;
float Visualizer::p3time = 120;
float Visualizer::p4time = 300;
bool Visualizer::descent1 = false;
bool Visualizer::descent2 = true;
// END TO BE DELETED


// Ring buffer
float* Visualizer::positions;
float* Visualizer::readPos;
float* Visualizer::writePos;
int Visualizer::numParticles;
int Visualizer::bufferSize;
int Visualizer::floatsPerBuffer;
bool Visualizer::ready = false;
boost::mutex Visualizer::bufLocks[3];

const float CAMERA_INIT_DIST_X = -10.0;
const float CAMERA_INIT_DIST_Y = 8.0;
const float CAMERA_INIT_DIST_Z = 15.0;

Visualizer::Visualizer()
{
	positions = NULL;
}

void Visualizer::triangle( const point4& a, const point4& b, const point4& c )
{
	vec3 normal = normalize(cross(b-a,c-b));
	if (Index >=  2 * NUM_VERTICES || Index >= NUM_VERTICES) // Gourad or Phong shading, average normals (planet 2 & 3)
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

point4 Visualizer::unit( const point4& p )
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

void Visualizer::divide_triangle( const point4& a, const point4& b, const point4& c, int count )
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

void Visualizer::tetrahedron( int count )
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

void Visualizer::init( void )
{
	// subdivide a tetrahedron into a sphere
		  
	tetrahedron( NUM_TIMES_TO_SUBDIVIDE );
	tetrahedron( NUM_TIMES_TO_SUBDIVIDE );
	tetrahedron( NUM_TIMES_TO_SUBDIVIDE );
	tetrahedron( NUM_TIMES_TO_SUBDIVIDE );
	tetrahedron( NUM_TIMES_TO_SUBDIVIDE );
 
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

void Visualizer::display( void )
{
	if(ready)
	{        
		float shadeType = 0;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		point4 light_position(position.x, position.y, position.z, 1);   // light is at camera position, always lighting the sun
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

		mat4 model_view;
		float x, y, z, mass;
		int bufNo = ((int)(readPos - positions) / floatsPerBuffer);
		
		// Wait for write pointer to be out of the buffer
		bufLocks[bufNo].lock();

		// Render each particle with OpenGL
		for(int i = 0; i < numParticles; i++, readPos += 4)
		{
			x = readPos[0];
			y = readPos[1];
			z = readPos[2];
			mass = 1.0f / 32.0f;
			model_view = LookAt(position, position + direction, up) * 
				Translate(x, y, z) * Scale(mass, mass, mass);
			glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
			glDrawArrays(GL_TRIANGLES, 4 * NUM_VERTICES, NUM_VERTICES);
		}
		if((readPos - positions) >= bufferSize)
		{
			readPos -= bufferSize;
		}
		bufLocks[bufNo].unlock();

		glutSwapBuffers();
	}
}

void Visualizer::reshape( int width, int height )
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

float theta = 0;
void Visualizer::keyboard( unsigned char key, int x, int y )
{
	point4 p;
	switch (key) 
	{
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
				position.x = at.x + CAMERA_INIT_DIST_X;
				position.y = at.y + CAMERA_INIT_DIST_Y;
				position.z = at.z + CAMERA_INIT_DIST_Z;
				up        = point4( 0.0, 1.0, 0.0, 0.0 );
				direction = at - position;                  
				zNear     = 1.0f;
				zFar      = 30.0f;
				break;
		default:
		break;                
	}                              
	glutPostRedisplay();
}

void Visualizer::SpecialKeys(int key, int x, int y) 
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

void Visualizer::Release(int key, int x, int y) 
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

int Visualizer::getConnection()
{
	// Variables for finding socket
	struct addrinfo hints, *servinfo, *ptr;
	int rv;
	int connectFD = -1;

	// Fill in hint struct
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// Variables for host and port
	string host = "mars.seas.ucla.edu";
	string port = "12345";

	// Get the candidate socket info structs
	if ((rv = getaddrinfo(host.c_str(), port.c_str(),
												&hints, &servinfo)) != 0)
	{
		string s = gai_strerror(rv);
		s += "\nHostname: " + host + "\nPort: " + port;
		cerr << "getaddrinfo: " << s << endl;
		return -1;
		//throw ConnectionException("getaddrinfo: " + s, -1, -1);
	}

	// Iterate through socket info struct list until one is found
	// which creates a socket and connects
	for (ptr = servinfo; ptr != NULL; ptr = ptr->ai_next)
	{
		connectFD = socket(ptr->ai_family, ptr->ai_socktype,
											ptr->ai_protocol);
		if (connectFD == -1)
		{
				continue;
		}
		if (connect(connectFD, ptr->ai_addr, ptr->ai_addrlen) == -1)
		{
				close(connectFD);
				continue;
		}
		break;
	}

	// Free allocated memory regardless of success
	freeaddrinfo(servinfo);

	// Check success
	if (ptr == NULL)
	{
		cerr << "Could not create the socket" << endl;
		return -1;
	}

	// Return connection file descriptor
	return connectFD;
}

void Visualizer::mainReadLoop(int connectFD)
{
	// Do until interrupted (socket closes/thread is interrupted)
	while(1)
	{
		int bytesCopied = 0;
		int bytesToCopy = floatsPerBuffer;
		
		int bufNo = ((int)(writePos - positions) / floatsPerBuffer);
		bufLocks[bufNo].lock();
		/*
		// Wait until there is room to write
		while(writePos < readPos && readPos < (writePos + floatsPerBuffer))
			{cerr << "IT" << endl;}
		*/
		// Copy read bytes into buffer
		while ((bytesCopied = read(connectFD, writePos, bytesToCopy)) < 
				bytesToCopy)
		{
			if(bytesCopied < 0)
			{
				// Connection was closed: Time to end
				close(connectFD);
				cout << "Connection closed by simulation" << endl <<
					"Shutting Down" << endl;
				exit(0);
			}
			bytesToCopy -= bytesCopied;
			writePos += bytesCopied;
		}
		bytesToCopy -= bytesCopied;
		writePos += bytesCopied;
		// If buffer would spill over
		if((writePos - positions) >= bufferSize)
		{
			writePos -= bufferSize;
		}
		bufLocks[bufNo].unlock();
	}
}

void Visualizer::registerWithSimulation()
{
	// Initialize connection file descriptor
	int connectFD = -1;
	try
	{
		// Get Connection to Simulation
		connectFD = getConnection();
		if(connectFD != -1) // (Valid connection)
		{
			// Read Number of particles from simulation and send acknowledgement
			int bytesRead = 0;
			char numberBuffer[10];
			if((bytesRead = read(connectFD, numberBuffer, 9)) > 0)
			{
				// Cap the read value and send back ACK
				numberBuffer[bytesRead] = '\0';
				int bytesWritten = 0;
				char ack[4] = "ACK";
				int bytesToWrite = 4;
				if((bytesWritten = write(connectFD, ack, bytesToWrite)) < 
					bytesToWrite)
				{
					cerr << "Could not send acknowledgement" 
							<< endl << "Shutting down" << endl;
					exit(1);
				}
				// Success: Number of particles retrieved and ACK sent
			}
			else
			{
				cerr << "Could not read number of particles" 
						<< endl << "Shutting down" << endl;
				exit(1);
			}
			// Create buffer of size 3 * number of particles
			// with 4 floats per particle
			numParticles = boost::lexical_cast<int>(numberBuffer);
			floatsPerBuffer = numParticles * 4;
			bufferSize = 3 * floatsPerBuffer;
			positions = new float[bufferSize];
			writePos = positions;
			readPos = positions;
		
			// Copy first buffer of bytes from simulation, then 
			// tell Visualizer it can begin to read the buffer
			int bytesCopied = 0;
			int bytesToCopy = floatsPerBuffer;
			while ((bytesCopied = read(connectFD, writePos, bytesToCopy)) < 
					bytesToCopy)
			{
				if(bytesCopied < 0)
				{
					// Connection was closed: Time to end
					close(connectFD);
					cout << "Connection closed by simulation" << endl <<
						"Shutting Down" << endl;
					exit(0);
				}
				bytesToCopy -= bytesCopied;
				writePos += bytesCopied;
			}
			bytesToCopy -= bytesCopied;
			writePos += bytesCopied;
			
			// Finally ready to begin visualizing
			ready = true;
			
			// Continue to read and populate the buffer in a loop
			mainReadLoop(connectFD);
		}
		else
		{
			cerr << "Could open connection to simulation" 
				<< endl << "Shutting down" << endl;
			exit(1);
		}
	}
	catch(boost::thread_interrupted thrdEx)
	{
		if(connectFD != -1)
			close(connectFD);
		if(positions != NULL)
			delete positions;
	}
}

void Visualizer::visualize()
{
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 50);
	glutInitWindowSize(800, 600);
	glutCreateWindow( "Visualizer" );
	
#ifndef __APPLE__  // include Mac OS X verions of headers
	glewInit();
#endif
		
	init();
	
	glutDisplayFunc(display);
	glutIdleFunc(glutPostRedisplay);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(SpecialKeys); // special function for arrow keys
	glutSpecialUpFunc(Release);
	
	glutMainLoop();
}

int main (int argc, char *argv[] )
{
	// Initialize glut and create Visualizer
	glutInit( &argc, argv );
	Visualizer visualizer;
	
	// Create thread for populating the buffer with data from the simulation
	boost::thread dataThread = boost::thread(boost::bind(
		&Visualizer::registerWithSimulation), &visualizer);
	
	// Run visualizer
	visualizer.visualize();
	
	// Terminate data thread
	dataThread.interrupt();
	
	// return success!
	return 0;
}


