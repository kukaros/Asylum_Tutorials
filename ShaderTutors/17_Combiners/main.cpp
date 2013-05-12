//*************************************************************************************************************
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "GdiPlus.lib")

#include <Windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>

#include "../extern/qglextensions.h"

// helper macros
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

// external variables
extern HDC hdc;
extern long screenwidth;
extern long screenheight;

// external functions
extern GLuint LoadTexture(const std::wstring& file);
extern GLuint LoadCubeTexture(const std::wstring& file);
extern GLuint CreateNormalizationCubemap();

// tutorial variables
GLuint texture1 = 0;
GLuint texture2 = 0;
GLuint texture3 = 0;
GLuint normcube = 0;
GLuint spheremap = 0;
GLuint cubemap = 0;

float lightpos[] = { 6, 3, 10, 1 };
float world[16];

struct CustomVertex
{
	float x, y, z;
	float nx, ny, nz;
	float u1, v1;		// texcoord0
	float u2, v2;		// texcoord1
	float u3, v3, w3;	// texcoord2 (tangent space light vector)
};

struct AdditionalData
{
	float tx, ty, tz;
	float bx, by, bz;
};

AdditionalData tangentframe[24];
static const int float_count = sizeof(CustomVertex) / sizeof(float);

float vertices[24 * float_count] =
{
	-0.5f, -0.5f, 0.5f, 0, -1, 0, 1, 0, 1, 0, 0, 0, 0,
	-0.5f, -0.5f, -0.5f, 0, -1, 0, 1, 1, 1, 1, 0, 0, 0,
	0.5f, -0.5f, -0.5f, 0, -1, 0, 0, 1, 0, 1, 0, 0, 0,
	0.5f, -0.5f, 0.5f, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0,

	-0.5f, 0.5f, 0.5f, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	0.5f, 0.5f, 0.5f, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0,
	0.5f, 0.5f, -0.5f, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0,
	-0.5f, 0.5f, -0.5f, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0,

	-0.5f, -0.5f, 0.5f, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	0.5f, -0.5f, 0.5f, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0,
	0.5f, 0.5f, 0.5f, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0,
	-0.5f, 0.5f, 0.5f, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0,

	0.5f, -0.5f, 0.5f, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0.5f, -0.5f, -0.5f, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0,
	0.5f, 0.5f, -0.5f, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0,
	0.5f, 0.5f, 0.5f, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0,

	0.5f, -0.5f, -0.5f, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0,
	-0.5f, -0.5f, -0.5f, 0, 0, -1, 1, 0, 1, 0, 0, 0, 0,
	-0.5f, 0.5f, -0.5f, 0, 0, -1, 1, 1, 1, 1, 0, 0, 0,
	0.5f, 0.5f, -0.5f, 0, 0, -1, 0, 1, 0, 1, 0, 0, 0,

	-0.5f, -0.5f, -0.5f, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	-0.5f, -0.5f, 0.5f, -1, 0, 0, 1, 0, 1, 0, 0, 0, 0,
	-0.5f, 0.5f, 0.5f, -1, 0, 0, 1, 1, 1, 1, 0, 0, 0,
	-0.5f, 0.5f, -0.5f, -1, 0, 0, 0, 1, 0, 1, 0, 0, 0
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

inline float dot(const float a[3], const float b[3]) {
	return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

inline void normalize(float a[3]) {
	float l = sqrtf(dot(a, a));

	a[0] /= l;
	a[1] /= l;
	a[2] /= l;
}

inline void cross(float out[3], const float a[3], const float b[3]) {
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

// tutorial functions
void DrawCube1();
void DrawCube2();
void DrawCube3();
void DrawCube4();
void DrawCube5();
void DrawCube6();

void CalculateTangentFrame()
{
	// calculate tangent frame and transform light vector
	unsigned short i1, i2, i3;
	float s1, s2, t1, t2, r;
	float a[3], c[3], s[3];
	CustomVertex* vert = (CustomVertex*)vertices;

	for( int i = 0; i < 24; ++i )
	{
		AdditionalData& tb = tangentframe[i];

		tb.bx = tb.by = tb.bz = 0;
		tb.tx = tb.ty = tb.tz = 0;
	}

	for( int i = 0; i < 36; i += 3 )
	{
		i1 = indices[i + 0];
		i2 = indices[i + 1];
		i3 = indices[i + 2];

		AdditionalData& tb1 = tangentframe[i1];
		AdditionalData& tb2 = tangentframe[i2];
		AdditionalData& tb3 = tangentframe[i3];

		const CustomVertex& v1 = vert[i1];
		const CustomVertex& v2 = vert[i2];
		const CustomVertex& v3 = vert[i3];

		// triangle edges
		a[0] = v2.x - v1.x;
		a[1] = v2.y - v1.y;
		a[2] = v2.z - v1.z;

		c[0] = v3.x - v1.x;
		c[1] = v3.y - v1.y;
		c[2] = v3.z - v1.z;

		// change in UV coordinates
		s1 = v2.u1 - v1.u1;
		s2 = v3.u1 - v1.u1;
		t1 = v2.u2 - v1.u2;
		t2 = v3.u2 - v1.u2;

		r = 1.0f / ((s1 * t2 - s2 * t1) + 0.0001f);

		s[0] = (t2 * a[0] - t1 * c[0]) * r;
		s[1] = (t2 * a[1] - t1 * c[1]) * r;
		s[2] = (t2 * a[2] - t1 * c[2]) * r;

		// accumulate
		tb1.tx += s[0];
		tb1.ty += s[1];
		tb1.tz += s[2];

		tb2.tx += s[0];
		tb2.ty += s[1];
		tb2.tz += s[2];

		tb3.tx += s[0];
		tb3.ty += s[1];
		tb3.tz += s[2];
	}

	for( int i = 0; i < 24; ++i )
	{
		AdditionalData& tb = tangentframe[i];
		const CustomVertex& v = vert[i];

		// Gramm-Smidth orthogonalize
		s1 = dot(&tb.tx, &v.nx);
		s2 = dot(&v.nx, &v.nx);

		s[0] = tb.tx - v.nx * s1 / s2;
		s[1] = tb.ty - v.ny * s1 / s2;
		s[2] = tb.tz - v.nz * s1 / s2;

		normalize(s);

		// calculate binormal
		tb.tx = s[0];
		tb.ty = s[1];
		tb.tz = s[2];

		cross(&tb.bx, &v.nx, &tb.tx);
		normalize(&tb.bx);
	}
}

bool InitScene()
{
	Quadron::qGLExtensions::QueryFeatures();

	// setup opengl
	//glClearColor(0.4f, 0.58f, 0.93f, 1.0f);
	glClearColor(0.153f, 0.18f, 0.16f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glShadeModel(GL_SMOOTH);

	glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0f);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	float spec[] = { 1, 1, 1, 1 };

	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);

	// setup scene
	texture1 = LoadTexture(L"../media/textures/gl_logo.png");

	if( 0 == texture1 )
	{
		MYERROR("InitScene(): Could not load texture");
		return false;
	}

	texture2 = LoadTexture(L"../media/textures/wood.jpg");

	if( 0 == texture2 )
	{
		MYERROR("InitScene(): Could not load texture");
		return false;
	}

	texture3 = LoadTexture(L"../media/textures/normalmap.jpg");

	if( 0 == texture3 )
	{
		MYERROR("InitScene(): Could not load texture");
		return false;
	}

	spheremap = LoadTexture(L"../media/textures/spheremap1.jpg");

	if( 0 == spheremap )
	{
		MYERROR("InitScene(): Could not load sphere map");
		return false;
	}

	cubemap = LoadCubeTexture(L"../media/textures/cubemap1.jpg");

	if( 0 == cubemap )
	{
		MYERROR("InitScene(): Could not load cube map");
		return false;
	}

	normcube = CreateNormalizationCubemap();
	CalculateTangentFrame();

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
	if( texture1 != 0 )
		glDeleteTextures(1, &texture1);

	if( texture2 != 0 )
		glDeleteTextures(1, &texture2);

	if( texture3 != 0 )
		glDeleteTextures(1, &texture3);

	if( spheremap != 0 )
		glDeleteTextures(1, &spheremap);

	if( cubemap != 0 )
		glDeleteTextures(1, &cubemap);

	if( normcube != 0 )
		glDeleteTextures(1, &normcube);

	texture1 = 0;
	texture2 = 0;
	texture3 = 0;
	spheremap = 0;
	normcube = 0;
}
//*************************************************************************************************************
void Update(float delta)
{
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// simplest solution...
	glRotatef(fmodf(time * 20.0f, 360.0f), 1, 0, 0);
	glRotatef(fmodf(time * 20.0f, 360.0f), 0, 1, 0);

	glGetFloatv(GL_MODELVIEW_MATRIX, world);
	glLoadIdentity();

	gluLookAt(0, 0, 2, 0, 0, 0, 0, 1, 0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	glRotatef(fmodf(time * 20.0f, 360.0f), 1, 0, 0);
	glRotatef(fmodf(time * 20.0f, 360.0f), 0, 1, 0);

	time += elapsedtime;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (float)(screenwidth / 3) / (float)(screenheight / 2), 0.1, 100.0);

	glEnable(GL_LIGHTING);

	glViewport(0, 0, screenwidth / 3, screenheight / 2);
	DrawCube1();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(screenwidth / 3, 0, screenwidth / 3, screenheight / 2);
	DrawCube2();

	glDisable(GL_BLEND);

	glViewport(0, screenheight / 2, screenwidth / 3, screenheight / 2);
	DrawCube3();

	glDisable(GL_LIGHTING);

	glViewport(screenwidth / 3, screenheight / 2, screenwidth / 3, screenheight / 2);
	DrawCube4();

	glViewport(2 * screenwidth / 3, 0, screenwidth / 3, screenheight / 2);
	DrawCube5();

	glViewport(2 * screenwidth / 3, screenheight / 2, screenwidth / 3, screenheight / 2);
	DrawCube6();

	// present
	SwapBuffers(hdc);
}
//*************************************************************************************************************
void DrawCube1()
{
	float* vert = &vertices[0];

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6);

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8);

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture1);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture2);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}
//*************************************************************************************************************
void DrawCube2()
{
	float* vert = &vertices[0];

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6);

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8);

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture2);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture1);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_INTERPOLATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GL_TEXTURE);

	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, GL_SRC_ALPHA);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}
//*************************************************************************************************************
void DrawCube3()
{
	float* vert = &vertices[0];

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6);

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8);

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture1);

	// multiply by 0.25 and add 0.5
	float scale[] = { 0.25f, 0.25f, 0.25f, 1 };
	float offset[] = { 0.5f, 0.5f, 0.5f, 0 };
	float gray[] = { 0.299f, 0.5f, 0.114f, 1 };

	gray[0] = gray[0] + 0.5f;
	gray[1] = gray[1] + 0.5f;
	gray[2] = gray[2] + 0.5f;

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture1); // doesnt matter

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, scale);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture1); // doesnt matter

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, offset);

	// grayscale
	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture1); // doesnt matter

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, gray);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE2);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}
//*************************************************************************************************************
void DrawCube4()
{
	// transform light vector to tangent space
	float ldir[3];
	float wpos[3];
	float wtan[3];
	float wbin[3];
	float wnorm[3];

	for( int i = 0; i < 24; ++i )
	{
		const AdditionalData& tb = tangentframe[i];
		CustomVertex& v = ((CustomVertex*)vertices)[i];

		wpos[0] = v.x * world[0] + v.y * world[4] + v.z * world[8] + world[12];
		wpos[1] = v.x * world[1] + v.y * world[5] + v.z * world[9] + world[13];
		wpos[2] = v.x * world[2] + v.y * world[6] + v.z * world[10] + world[14];

		wtan[0] = tb.tx * world[0] + tb.ty * world[4] + tb.tz * world[8];
		wtan[1] = tb.tx * world[1] + tb.ty * world[5] + tb.tz * world[9];
		wtan[2] = tb.tx * world[2] + tb.ty * world[6] + tb.tz * world[10];

		wbin[0] = tb.bx * world[0] + tb.by * world[4] + tb.bz * world[8];
		wbin[1] = tb.bx * world[1] + tb.by * world[5] + tb.bz * world[9];
		wbin[2] = tb.bx * world[2] + tb.by * world[6] + tb.bz * world[10];

		// we know this is not correct, but ignore for now
		wnorm[0] = v.nx * world[0] + v.ny * world[4] + v.nz * world[8];
		wnorm[1] = v.nx * world[1] + v.ny * world[5] + v.nz * world[9];
		wnorm[2] = v.nx * world[2] + v.ny * world[6] + v.nz * world[10];

		ldir[0] = lightpos[0] - wpos[0];
		ldir[1] = lightpos[1] - wpos[1];
		ldir[2] = lightpos[2] - wpos[2];

		v.u3 = dot(wtan, ldir);
		v.v3 = dot(wbin, ldir);
		v.w3 = dot(wnorm, ldir);
	}

	float* vert = &vertices[0];

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8); // texcoord0 (normtex)

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(3, GL_FLOAT, sizeof(CustomVertex), vert + 10); // texcoord1 (lightts)

	glClientActiveTexture(GL_TEXTURE2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6); // texcoord2 (basetex)

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture3); // normal map

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, normcube); // normalization cube map

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture2); // base color

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glDisable(GL_TEXTURE_CUBE_MAP);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}
//*************************************************************************************************************
void DrawCube5()
{
	float* vert = &vertices[0];

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8);

	glClientActiveTexture(GL_TEXTURE2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6);

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glBindTexture(GL_TEXTURE_2D, spheremap);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture2);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture1);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
}
//*************************************************************************************************************
void DrawCube6()
{
	float* vert = &vertices[0];

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// enable vertex attrib pointers
	glVertexPointer(3, GL_FLOAT, sizeof(CustomVertex), vert);
	glNormalPointer(GL_FLOAT, sizeof(CustomVertex), vert + 3);

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 8);

	glClientActiveTexture(GL_TEXTURE2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, sizeof(CustomVertex), vert + 6);

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture2);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture1);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);

	// draw
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indices);
	
	// unbind textures
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// disable attrib pointers
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
}
//*************************************************************************************************************
