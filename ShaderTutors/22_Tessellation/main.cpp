//*************************************************************************************************************
#include <Windows.h>
#include <iostream>

#include "../common/glext.h"

// helper macros
#define TITLE				"Shader tutorial 22.3: Tessellating NURBS surfaces"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define M_PI				3.141592f

// external variables
extern HWND		hwnd;
extern HDC		hdc;
extern long		screenwidth;
extern long		screenheight;
extern short	mousedx;
extern short	mousedy;
extern short	mousedown;

// sample variables
array_state<float, 2> cameraangle;

void APIENTRY ReportGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userdata)
{
	if( type >= GL_DEBUG_TYPE_ERROR && type <= GL_DEBUG_TYPE_PERFORMANCE )
	{
		if( source == GL_DEBUG_SOURCE_API )
			std::cout << "GL(" << severity << "): ";
		else if( source == GL_DEBUG_SOURCE_SHADER_COMPILER )
			std::cout << "GLSL(" << severity << "): ";
		else
			std::cout << "OTHER(" << severity << "): ";

		std::cout << id << ": " << message << "\n";
	}
}

bool InitScene()
{
	SetWindowText(hwnd, TITLE);
	Quadron::qGLExtensions::QueryFeatures(hdc);

	if( !Quadron::qGLExtensions::ARB_tessellation_shader )
		return false;

#ifdef _DEBUG
	if( Quadron::qGLExtensions::ARB_debug_output )
	{
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
		glDebugMessageCallback(ReportGLError, 0);
	}
#endif

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	//glClearColor(1, 1, 1, 1);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	float angles[2] = { -0.25f, 0.7f };
	cameraangle = angles;

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
	GLKillAnyRogueObject();
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
}
//*************************************************************************************************************
void Update(float delta)
{
	cameraangle.prev[0] = cameraangle.curr[0];
	cameraangle.prev[1] = cameraangle.curr[1];

	if( mousedown == 1 )
	{
		cameraangle.curr[0] += mousedx * 0.004f;
		cameraangle.curr[1] += mousedy * 0.004f;
	}

	// clamp to [-pi, pi]
	if( cameraangle.curr[1] >= 1.5f )
		cameraangle.curr[1] = 1.5f;

	if( cameraangle.curr[1] <= -1.5f )
		cameraangle.curr[1] = -1.5f;
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	float view[16];
	float proj[16];
	float viewproj[16];
	float eye[3] = { 0, 0, 8 };
	float look[3] = { 0, 0, 0 };
	float up[3] = { 0, 1, 0 };
	float orient[2];

	cameraangle.smooth(orient, alpha);

	GLMatrixRotationYawPitchRoll(view, orient[0], orient[1], 0);
	GLVec3Transform(eye, eye, view);

	GLMatrixPerspectiveRH(proj, (60.0f * 3.14159f) / 180.f,  (float)screenwidth / (float)screenheight, 0.1f, 50.0f);

	GLMatrixLookAtRH(view, eye, look, up);
	GLMatrixMultiply(viewproj, view, proj);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

#ifdef _DEBUG
	// check errors
	GLenum err = glGetError();

	if( err != GL_NO_ERROR )
		std::cout << "Error\n";
#endif

	SwapBuffers(hdc);
}
//*************************************************************************************************************
