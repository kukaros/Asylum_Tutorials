//*************************************************************************************************************
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "GdiPlus.lib")

#include <Windows.h>
#include <GdiPlus.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>

#include "../IntelTest/qgl2extensions.h"

// helper macros
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

// external variables
extern HDC hdc;
extern long screenwidth;
extern long screenheight;

// tutorial variables
GLuint texid				= 0;
GLuint vertexbuffer			= 0;
GLuint indexbuffer			= 0;
GLuint vertexshader			= 0;
GLuint pixelshader			= 0;
GLuint program				= 0;
GLuint vertexdecl			= 0;

GLuint attrib_Position		= 0;
GLuint attrib_Normal		= 0;

GLuint uniform_matWorld		= 0;
GLuint uniform_matWorldInv	= 0;
GLuint uniform_matViewProj	= 0;

const char* vscode =
{
	"#version 110\n"

	"attribute vec3 my_Position;\n"
	"attribute vec3 my_Normal;\n"

	"uniform mat4 matWorld;\n"
	"uniform mat4 matWorldInv;\n"
	"uniform mat4 matViewProj;\n"

	"varying vec3 wnorm;\n"

	"void main()\n"
	"{\n"
		"vec4 wpos = matWorld * vec4(my_Position, 1);\n"

		"wnorm = (vec4(my_Normal, 0) * matWorldInv).xyz;\n"
		"gl_Position = matViewProj * wpos;\n"
	"}\n"
};

const char* pscode =
{
	"#version 110\n"

	"varying vec3 wnorm;\n"

	"void main()\n"
	"{\n"
		"gl_FragColor = vec4(1, 1, 1, 1);\n"
	"}\n"
};

struct CommonVertex
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};

float vertices[24 * sizeof(CommonVertex)] =
{
	-0.5f, -0.5f, 0.5f, 0, -1, 0, 1, 0,
	-0.5f, -0.5f, -0.5f, 0, -1, 0, 1, 1,
	0.5f, -0.5f, -0.5f, 0, -1, 0, 0, 1,
	0.5f, -0.5f, 0.5f, 0, -1, 0, 0, 0,

	-0.5f, 0.5f, 0.5f, 0, 1, 0, 0, 0,
	0.5f, 0.5f, 0.5f, 0, 1, 0, 1, 0,
	0.5f, 0.5f, -0.5f, 0, 1, 0, 1, 1,
	-0.5f, 0.5f, -0.5f, 0, 1, 0, 0, 1,

	-0.5f, -0.5f, 0.5f, 0, 0, 1, 0, 0,
	0.5f, -0.5f, 0.5f, 0, 0, 1, 1, 0,
	0.5f, 0.5f, 0.5f, 0, 0, 1, 1, 1,
	-0.5f, 0.5f, 0.5f, 0, 0, 1, 0, 1,

	0.5f, -0.5f, 0.5f, 1, 0, 0, 0, 0,
	0.5f, -0.5f, -0.5f, 1, 0, 0, 1, 0,
	0.5f, 0.5f, -0.5f, 1, 0, 0, 1, 1,
	0.5f, 0.5f, 0.5f, 1, 0, 0, 0, 1,

	0.5f, -0.5f, -0.5f, 0, 0, -1, 0, 0,
	-0.5f, -0.5f, -0.5f, 0, 0, -1, 1, 0,
	-0.5f, 0.5f, -0.5f, 0, 0, -1, 1, 1,
	0.5f, 0.5f, -0.5f, 0, 0, -1, 0, 1,

	-0.5f, -0.5f, -0.5f, -1, 0, 0, 0, 0,
	-0.5f, -0.5f, 0.5f, -1, 0, 0, 1, 0,
	-0.5f, 0.5f, 0.5f, -1, 0, 0, 1, 1,
	-0.5f, 0.5f, -0.5f, -1, 0, 0, 0, 1
};

unsigned short indices[36] =
{
	0, 1, 2, 2, 3, 0, 
	4, 5, 6, 6, 7, 4,
	8, 9, 10, 10, 11, 8,
	12, 13, 14, 14, 15, 12,
	16, 17, 18, 18, 19, 16,
	20, 21, 22, 22, 23, 20
};

inline float Dot(float a[3], float b[3]) {
	return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

inline float Length(float a[3]) {
	return sqrtf(Dot(a, a));
}

void Normalize(float a[3])
{
	float il = 1.0f / Length(a);

	a[0] *= il;
	a[1] *= il;
	a[2] *= il;
}

void Cross(float out[3], float a[3], float b[3])
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

void LookAtRH(float out[16], float eye[3], float look[3], float up[3])
{
	float x[3], y[3], z[3];

	z[0] = look[0] - eye[0];
	z[1] = look[1] - eye[1];
	z[2] = look[2] - eye[2];

	Normalize(z);
	Cross(x, z, up);

	Normalize(x);
	Cross(y, x, z);

	out[0] = x[0];		out[1] = y[0];		out[2] = -z[0];		out[3] = 0.0f;
	out[4] = x[1];		out[5] = y[1];		out[6] = -z[1];		out[7] = 0.0f;
	out[8] = x[2];		out[9] = y[2];		out[10] = -z[2];	out[11] = 0.0f;

	out[12] = Dot(x, eye);
	out[13] = Dot(y, eye);
	out[14] = Dot(z, eye);
	out[15] = 1.0f;
}

void PerspectiveRH(float out[16], float fovy, float aspect, float nearplane, float farplane)
{
	out[5] = 1.0f / tanf(fovy / 2);
	out[0] = out[5] / aspect;

	out[1] = out[2] = out[3] = 0;
	out[4] = out[6] = out[7] = 0;
	out[8] = out[9] = 0;
	out[12] = out[13] = out[15] = 0;

	out[11] = -1.0f;

	out[10] = (farplane + nearplane) / (nearplane - farplane);
	out[14] = 2 * farplane * nearplane / (nearplane - farplane);
}

void Multiply(float out[16], float a[16], float b[16])
{
	out[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
	out[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
	out[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
	out[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

	out[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
	out[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
	out[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
	out[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

	out[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
	out[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
	out[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
	out[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

	out[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
	out[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
	out[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
	out[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
}

void Identity(float out[16])
{
	memset(out, 0, 16 * sizeof(float));
	out[0] = out[5] = out[10] = out[15] = 1;
}

bool InitScene()
{
	Quadron::qGL2Extensions::QueryFeatures();

	// setup opengl
	//glClearColor(0.4f, 0.58f, 0.93f, 1.0f);
	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	// mesh
	glGenBuffers(1, &vertexbuffer);
	glGenBuffers(1, &indexbuffer);

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(CommonVertex), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(unsigned short), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// shader
	GLint length;
	GLint success = GL_FALSE;

	program = glCreateProgram();
	vertexshader = glCreateShader(GL_VERTEX_SHADER);
	pixelshader = glCreateShader(GL_FRAGMENT_SHADER);

	length = strlen(vscode);
	glShaderSource(vertexshader, 1, &vscode, &length);
	glCompileShader(vertexshader);
	glGetShaderiv(vertexshader, GL_COMPILE_STATUS, &success);

	if( success != GL_TRUE )
	{
		MYERROR("InitScene(): Could not compile vertex shader");
		return false;
	}

	length = strlen(pscode);
	glShaderSource(pixelshader, 1, &pscode, &length);
	glCompileShader(pixelshader);
	glGetShaderiv(vertexshader, GL_COMPILE_STATUS, &success);

	if( success != GL_TRUE )
	{
		MYERROR("InitScene(): Could not compile pixel shader");
		return false;
	}

	glAttachShader(program, vertexshader);
	glAttachShader(program, pixelshader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if( success != GL_TRUE )
	{
		MYERROR("InitScene(): Could not link shader");
		return false;
	}

	glUseProgram(program);
	{
		attrib_Position = glGetAttribLocation(program, "my_Position");
		attrib_Normal = glGetAttribLocation(program, "my_Normal");

		uniform_matWorld = glGetUniformLocation(program, "matWorld");
		uniform_matWorldInv = glGetUniformLocation(program, "matWorldInv");
		uniform_matViewProj = glGetUniformLocation(program, "matViewProj");
	}
	glUseProgram(0);

	// vertex decl
	glGenVertexArrays(1, &vertexdecl);
	glBindVertexArray(vertexdecl);
	{
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);

		glEnableVertexAttribArray(attrib_Position);
		glEnableVertexAttribArray(attrib_Normal);

		glVertexAttribPointer(attrib_Position, 3, GL_FLOAT, GL_FALSE, sizeof(CommonVertex), 0);
		glVertexAttribPointer(attrib_Normal, 3, GL_FLOAT, GL_FALSE, sizeof(CommonVertex), (const GLvoid*)(3 * sizeof(float)));
	}
	glBindVertexArray(0);

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
	if( texid != 0 )
		glDeleteTextures(1, &texid);

	if( vertexbuffer != 0 )
		glDeleteBuffers(1, &vertexbuffer);

	if( indexbuffer != 0 )
		glDeleteBuffers(1, &indexbuffer);

	if( program != 0 )
	{
		glUseProgram(0);

		if( vertexshader != 0 )
		{
			glDetachShader(program, vertexshader);
			glDeleteShader(vertexshader);
		}

		if( pixelshader != 0 )
		{
			glDetachShader(program, pixelshader);
			glDeleteShader(pixelshader);
		}

		glDeleteProgram(program);
	}

	if( vertexdecl != 0 )
		glDeleteVertexArrays(1, &vertexdecl);

	texid = 0;
	vertexbuffer = 0;
	indexbuffer = 0;
	vertexshader = 0;
	pixelshader = 0;
	program = 0;
	vertexdecl = 0;
}
//*************************************************************************************************************
void Update(float delta)
{
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	float* vert			= &vertices[0];
	float lightpos[4]	= { 6, 3, 10, 1 };
	float eye[3]		= { 0, 0, 2 };
	float look[3]		= { 0, 0, 0 };
	float up[3]			= { 0, 1, 0 };

	float view[16];
	float proj[16];
	float world[16];
	float viewproj[16];

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	LookAtRH(view, eye, look, up);
	PerspectiveRH(proj, (60.0f * 3.14159f) / 180.f,  (float)screenwidth / (float)screenheight, 0.1f, 100.0f);
	
	Multiply(viewproj, view, proj);
	Identity(world);

	//glRotatef(fmodf(time * 20.0f, 360.0f), 1, 0, 0);
	//glRotatef(fmodf(time * 20.0f, 360.0f), 0, 1, 0);

	time += elapsedtime;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(program);
	glUniformMatrix4fv(uniform_matWorld, 1, false, world);
	glUniformMatrix4fv(uniform_matViewProj, 1, false, viewproj);
	{
		glBindVertexArray(vertexdecl);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
		glBindVertexArray(0);
	}
	glUseProgram(0);

	glDisable(GL_BLEND);

	// check errors
	GLenum err = glGetError();

	if( err != GL_NO_ERROR )
		std::cout << "Error\n";

	SwapBuffers(hdc);
}
//*************************************************************************************************************
