//*************************************************************************************************************
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "GdiPlus.lib")

#include <Windows.h>
#include <GdiPlus.h>
#include <iostream>

#include "../common/glext.h"

// helper macros
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

// external variables
extern HDC hdc;
extern long screenwidth;
extern long screenheight;

// tutorial variables
OpenGLMesh*			mesh			= 0;
OpenGLEffect*		lightcull		= 0;
OpenGLEffect*		lightaccum		= 0;
OpenGLFramebuffer*	framebuffer		= 0;
GLuint				texture			= 0;
GLuint				headbuffer		= 0;	// head of linked lists
GLuint				nodebuffer		= 0;	// nodes of linked lists
GLuint				workgroupsx		= 0;
GLuint				workgroupsy		= 0;

bool InitScene()
{
	Quadron::qGLExtensions::QueryFeatures();

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	OpenGLMaterial* materials = 0;
	GLuint nummaterials = 0;

	bool ok = GLLoadMeshFromQM("../media/meshes/teapot.qm", &materials, &nummaterials, &mesh);

	if( !ok )
	{
		MYERROR("Could not load mesh");
		return false;
	}

	delete[] materials;

	framebuffer = new OpenGLFramebuffer(screenwidth, screenheight);

	framebuffer->AttachRenderbuffer(GL_COLOR_ATTACHMENT0, GLFMT_A8R8G8B8);
	framebuffer->AttachTexture(GL_DEPTH_ATTACHMENT, GLFMT_D32F);
	
	if( !framebuffer->Validate() )
		return false;

	// texture
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	{
		unsigned int* data = new unsigned int[128 * 128];
		memset(data, 0xff, 128 * 128 * 4);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
		delete[] data;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	// buffers
	workgroupsx = (screenwidth + (screenwidth % 16)) / 16;
	workgroupsy = (screenheight + (screenheight % 16)) / 16;

	size_t numtiles = workgroupsx * workgroupsy;
	size_t headsize = 8;	// start, count
	size_t nodesize = 8;	// light index + next

	glGenBuffers(1, &headbuffer);
	glGenBuffers(1, &nodebuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, headbuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numtiles * headsize, 0, GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodebuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numtiles * nodesize * 1024, 0, GL_STATIC_DRAW);	// 2 MB

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// light accumulation shader
	if( !(ok = GLCreateEffectFromFile("../media/shadersGL/lightaccum.vert", "../media/shadersGL/lightaccum.frag", &lightaccum)) )
	{
		MYERROR("Could not load light accumulation shader");
		return false;
	}

	// light culling shader
	if( !(ok = GLCreateComputeProgramFromFile("../media/shadersGL/lightcull.comp", 0, &lightcull)) )
	{
		MYERROR("Could not load light culling shader");
		return false;
	}

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
	if( lightcull )
		delete lightcull;

	if( lightaccum )
		delete lightaccum;

	if( mesh )
		delete mesh;

	if( framebuffer )
		delete framebuffer;

	if( texture )
		glDeleteTextures(1, &texture);

	if( headbuffer )
		glDeleteBuffers(1, &headbuffer);

	if( nodebuffer )
		glDeleteBuffers(1, &nodebuffer);

	lightcull = 0;
	lightaccum = 0;
	mesh = 0;
	texture = 0;
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
	float eye[3]		= { 0, 6, 8 };
	float look[3]		= { 0, 0, 0 };
	float up[3]			= { 0, 1, 0 };

	float view[16];
	float proj[16];
	float world[16];
	float viewproj[16];

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	GLMatrixLookAtRH(view, eye, look, up);
	GLMatrixPerspectiveRH(proj, (60.0f * 3.14159f) / 180.f,  (float)screenwidth / (float)screenheight, 0.1f, 100.0f);
	
	GLMatrixMultiply(viewproj, view, proj);
	GLMatrixIdentity(world);

	time += elapsedtime;

	// cull lights
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, headbuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, nodebuffer);

	lightcull->Begin();
	{
		glDispatchCompute(workgroupsx, workgroupsy, 1);
	}
	lightcull->End();

	// render
	//lightaccum->SetVector("eyePos", eye);
	//lightaccum->SetVector("lightPos", lightpos);
	lightaccum->SetMatrix("matViewProj", viewproj);
	//lightaccum->SetInt("sampler0", 0);

	lightaccum->Begin();
	{
		glBindTexture(GL_TEXTURE_2D, texture);

		for( int i = -1; i < 2; ++i )
		{
			for( int j = -1; j < 2; ++j )
			{
				world[12] = -0.108f + i * 3;
				world[13] = -0.108f;
				world[14] =  j * 3.0f;

				lightaccum->SetMatrix("matWorld", world);
				lightaccum->CommitChanges();

				mesh->DrawSubset(0);
			}
		}

		glBindTexture(GL_TEXTURE_2D, 0);
	}
	lightaccum->End();

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

	// check errors
	GLenum err = glGetError();

	if( err != GL_NO_ERROR )
		std::cout << "Error\n";

	SwapBuffers(hdc);
}
//*************************************************************************************************************
