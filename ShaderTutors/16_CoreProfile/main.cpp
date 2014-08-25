//*************************************************************************************************************
#include <Windows.h>
#include <GdiPlus.h>
#include <iostream>

#include "../common/glext.h"

// helper macros
#define TITLE				"Shader tutorial 16: OpenGL core profile"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// external variables
extern HWND		hwnd;
extern HDC		hdc;
extern long		screenwidth;
extern long		screenheight;

// tutorial variables
OpenGLMesh* mesh = 0;

GLuint vertexshader			= 0;
GLuint pixelshader			= 0;
GLuint program				= 0;

GLuint uniform_lightPos		= 0;
GLuint uniform_eyePos		= 0;
GLuint uniform_matWorld		= 0;
GLuint uniform_matViewProj	= 0;

const char* vscode =
{
	"#version 150\n"

	"in vec3 my_Position;\n"
	"in vec3 my_Normal;\n"

	"uniform mat4 matWorld;\n"
	"uniform mat4 matWorldInv;\n"
	"uniform mat4 matViewProj;\n"

	"uniform vec4 lightPos;\n"
	"uniform vec4 eyePos;\n"

	"out vec3 wnorm;\n"
	"out vec3 vdir;\n"
	"out vec3 ldir;\n"

	"void main()\n"
	"{\n"
	"	vec4 wpos = matWorld * vec4(my_Position, 1);\n"

	"	ldir = lightPos.xyz - wpos.xyz;\n"
	"	vdir = eyePos.xyz - wpos.xyz;\n"

	"	wnorm = (matWorld * vec4(my_Normal, 0)).xyz;\n"
	"	gl_Position = matViewProj * wpos;\n"
	"}\n"
};

const char* pscode =
{
	"#version 150\n"

	"in vec3 wnorm;\n"
	"in vec3 vdir;\n"
	"in vec3 ldir;\n"
	
	"out vec4 outColor;\n"

	"void main()\n"
	"{\n"
	"	vec3 n = normalize(wnorm);\n"
	"	vec3 l = normalize(ldir);\n"
	"	vec3 v = normalize(vdir);\n"
	"	vec3 h = normalize(v + l);\n"

	"	float d = clamp(dot(l, n), 0.0, 1.0);\n"
	"	float s = clamp(dot(h, n), 0.0, 1.0);\n"

	"	s = pow(s, 80.0);\n"

	"	outColor.rgb = vec3(d, d, d) + vec3(s, s, s);\n"
	"	outColor.a = 1.0;\n"
	"}\n"
};

bool InitScene()
{
	SetWindowText(hwnd, TITLE);
	Quadron::qGLExtensions::QueryFeatures();

	// setup opengl
	//glClearColor(0.4f, 0.58f, 0.93f, 1.0f);
	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	OpenGLMaterial* materials = 0;
	GLuint nummaterials = 0;

	bool ok = GLCreateMeshFromQM("../media/meshes/teapot.qm", &materials, &nummaterials, &mesh);

	if( !ok )
	{
		MYERROR("Could not load mesh");
		return false;
	}

	delete[] materials;

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

	glBindAttribLocation(program, GLDECLUSAGE_POSITION, "my_Position");
	glBindAttribLocation(program, GLDECLUSAGE_NORMAL, "my_Normal");
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if( success != GL_TRUE )
	{
		MYERROR("InitScene(): Could not link shader");
		return false;
	}

	glUseProgram(program);
	{
		uniform_lightPos = glGetUniformLocation(program, "lightPos");
		uniform_eyePos = glGetUniformLocation(program, "eyePos");
		uniform_matWorld = glGetUniformLocation(program, "matWorld");
		uniform_matViewProj = glGetUniformLocation(program, "matViewProj");
	}
	glUseProgram(0);

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
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

	if( mesh )
		delete mesh;

	mesh = 0;
	vertexshader = 0;
	pixelshader = 0;
	program = 0;
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
}
//*************************************************************************************************************
void MouseMove()
{
}
//*************************************************************************************************************
void Update(float delta)
{
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	float lightpos[4]	= { 6, 3, 10, 1 };
	float eye[3]		= { 0, 0, 3 };
	float look[3]		= { 0, 0, 0 };
	float up[3]			= { 0, 1, 0 };

	float view[16];
	float proj[16];
	float world[16];
	float viewproj[16];
	float tmp1[16];
	float tmp2[16];

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	GLMatrixLookAtRH(view, eye, look, up);
	GLMatrixPerspectiveRH(proj, (60.0f * 3.14159f) / 180.f,  (float)screenwidth / (float)screenheight, 0.1f, 100.0f);
	
	GLMatrixMultiply(viewproj, view, proj);

	//calculate world matrix
	GLMatrixIdentity(tmp2);
	
	tmp2[12] = -0.108f;		// offset with bb center
	tmp2[13] = -0.7875f;	// offset with bb center

	GLMatrixRotationAxis(tmp1, fmodf(time * 20.0f, 360.0f) * (3.14152f / 180.0f), 1, 0, 0);
	GLMatrixMultiply(world, tmp2, tmp1);

	GLMatrixRotationAxis(tmp2, fmodf(time * 20.0f, 360.0f) * (3.14152f / 180.0f), 0, 1, 0);
	GLMatrixMultiply(world, world, tmp2);

	// render
	time += elapsedtime;

	glUseProgram(program);
	glUniform4fv(uniform_eyePos, 1, eye);
	glUniform4fv(uniform_lightPos, 1, lightpos);
	glUniformMatrix4fv(uniform_matWorld, 1, false, world);
	glUniformMatrix4fv(uniform_matViewProj, 1, false, viewproj);
	{
		mesh->DrawSubset(0);
	}
	glUseProgram(0);

	// check errors
	GLenum err = glGetError();

	if( err != GL_NO_ERROR )
		std::cout << "Error\n";

	SwapBuffers(hdc);
}
//*************************************************************************************************************
