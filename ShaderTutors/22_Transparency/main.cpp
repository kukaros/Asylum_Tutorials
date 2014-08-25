//*************************************************************************************************************
#include <Windows.h>
#include <iostream>

#include "../common/glext.h"

// TODO:
// - ARB_clear_buffer_object
// - ARB_robustness_isolation
// - elszall

// helper macros
#define TITLE				"Shader sample 22.2: Order independent transparency"
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

// sample structures
struct SceneObject
{
	int type;			// 0 for box, 1 for dragon
	float position[3];
	float scale[3];
	float angle;
	OpenGLColor color;
};

// sample variables
OpenGLMesh*			box				= 0;
OpenGLMesh*			dragon			= 0;
OpenGLEffect*		init			= 0;
OpenGLEffect*		collect			= 0;
OpenGLEffect*		render			= 0;
OpenGLScreenQuad*	screenquad		= 0;
OpenGLAABox			scenebox;

GLuint				white;
GLuint				headbuffer		= 0;
GLuint				nodebuffer		= 0;
GLuint				counterbuffer	= 0;

array_state<float, 2> cameraangle;

SceneObject objects[] =
{
	{ 0, { 0, -0.35f, 0 }, { 15, 0.5f, 15 }, 0, OpenGLColor(1, 1, 0, 0.75f) },

	{ 1, { -1, -0.1f, 2.5f }, { 0.3f, 0.3f, 0.3f }, -M_PI / 8, OpenGLColor(1, 0, 1, 0.5f) },
	{ 1, { 2, -0.1f, 0 }, { 0.3f, 0.3f, 0.3f }, M_PI / -2 + M_PI / -6, OpenGLColor(0, 1, 1, 0.5f) },
	{ 1, { -2, -0.1f, -2 }, { 0.3f, 0.3f, 0.3f }, M_PI / -4, OpenGLColor(1, 0, 0, 0.5f) }
};

const int numobjects = sizeof(objects) / sizeof(SceneObject);

static void APIENTRY ReportGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userdata)
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

	if( !Quadron::qGLExtensions::ARB_shader_storage_buffer_object )
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

	screenquad = new OpenGLScreenQuad();

	// load objects
	OpenGLMaterial* materials = 0;
	GLuint nummaterials = 0;

	if( !GLCreateMeshFromQM("../media/meshes/cube.qm", &materials, &nummaterials, &box) )
	{
		MYERROR("Could not load box");
		return false;
	}

	delete[] materials;

	if( !GLCreateMeshFromQM("../media/meshes/dragon.qm", &materials, &nummaterials, &dragon) )
	{
		MYERROR("Could not load teapot");
		return false;
	}

	delete[] materials;

	// create texture
	glGenTextures(1, &white);
	glBindTexture(GL_TEXTURE_2D, white);
	{
		unsigned int wondercolor = 0xffffffff;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &wondercolor);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	// create buffers
	size_t headsize = 16;	// start, count, pad, pad
	size_t nodesize = 16;	// color, depth, next, pad
	size_t numlists = screenwidth * screenheight;

	glGenBuffers(1, &headbuffer);
	glGenBuffers(1, &nodebuffer);
	glGenBuffers(1, &counterbuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, headbuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numlists * headsize, 0, GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodebuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numlists * 4 * nodesize, 0, GL_STATIC_DRAW);	// 120 MB @ 1080p

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterbuffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), 0, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	// calculate scene bounding box
	OpenGLAABox tmpbox;
	float world[16];
	float tmp[16];

	GLMatrixIdentity(world);

	for( int i = 0; i < numobjects; ++i )
	{
		const SceneObject& obj = objects[i];

		// scaling * rotation * translation
		GLMatrixScaling(tmp, obj.scale[0], obj.scale[1], obj.scale[2]);
		GLMatrixRotationAxis(world, obj.angle, 0, 1, 0);
		GLMatrixMultiply(world, tmp, world);

		GLMatrixTranslation(tmp, obj.position[0], obj.position[1], obj.position[2]);
		GLMatrixMultiply(world, world, tmp);

		if( obj.type == 0 )
			tmpbox = box->GetBoundingBox();
		else if( obj.type == 1 )
			tmpbox = dragon->GetBoundingBox();

		tmpbox.TransformAxisAligned(world);

		scenebox.Add(tmpbox.Min);
		scenebox.Add(tmpbox.Max);
	}

	// head pointer initializer
	if( !GLCreateEffectFromFile("../media/shadersGL/basic2D.vert", 0, "../media/shadersGL/initheadpointers.frag", &init) )
	{
		MYERROR("Could not load initializer shader");
		return false;
	}

	// fragment collector shader
	if( !GLCreateEffectFromFile("../media/shadersGL/ambient.vert", 0, "../media/shadersGL/collectfragments.frag", &collect) )
	{
		MYERROR("Could not load collector shader");
		return false;
	}

	// renderer shader
	if( !GLCreateEffectFromFile("../media/shadersGL/basic2D.vert", 0, "../media/shadersGL/renderfragments.frag", &render) )
	{
		MYERROR("Could not load rendering shader");
		return false;
	}

	float angles[2] = { -0.25f, 0.7f };
	cameraangle = angles;

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
	if( screenquad )
		delete screenquad;

	if( box )
		delete box;

	if( dragon )
		delete dragon;

	if( init )
		delete init;

	if( collect )
		delete collect;

	if( render )
		delete render;

	if( white )
		glDeleteTextures(1, &white);

	if( headbuffer )
		glDeleteBuffers(1, &headbuffer);

	if( nodebuffer )
		glDeleteBuffers(1, &nodebuffer);

	if( counterbuffer )
		glDeleteBuffers(1, &counterbuffer);

	GLKillAnyRogueObject();
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
	float world[16];
	float tmp[16];
	float view[16];
	float proj[16];
	float viewproj[16];
	float eye[3] = { 0, 0, 8 };
	float look[3] = { 0, 0, 0 };
	float up[3] = { 0, 1, 0 };
	float clipplanes[2];
	float orient[2];
	float texuv[2] = { 1, 1 };

	cameraangle.smooth(orient, alpha);

	GLMatrixRotationYawPitchRoll(view, orient[0], orient[1], 0);
	GLVec3Transform(eye, eye, view);

	GLFitToBox(clipplanes[0], clipplanes[1], eye, look, scenebox);
	GLMatrixPerspectiveRH(proj, (60.0f * 3.14159f) / 180.f,  (float)screenwidth / (float)screenheight, clipplanes[0], clipplanes[1]);

	GLMatrixLookAtRH(view, eye, look, up);
	GLMatrixMultiply(viewproj, view, proj);

	// STEP 1: initialize header pointer buffer
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, headbuffer);

	init->SetInt("screenWidth", screenwidth);

	init->Begin();
	{
		screenquad->Draw();
	}
	init->End();

	// STEP 2: collect transparent fragments into lists
	glBindTexture(GL_TEXTURE_2D, white);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterbuffer);

	GLuint* counter = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
	*counter = 0;

	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, nodebuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counterbuffer);

	collect->SetMatrix("matViewProj", viewproj);
	collect->SetVector("uv", texuv);
	collect->SetInt("screenWidth", screenwidth);

	collect->Begin();
	{
		GLMatrixIdentity(world);

		for( int i = 0; i < numobjects; ++i )
		{
			const SceneObject& obj = objects[i];

			// scaling * rotation * translation
			GLMatrixScaling(tmp, obj.scale[0], obj.scale[1], obj.scale[2]);
			GLMatrixRotationAxis(world, obj.angle, 0, 1, 0);
			GLMatrixMultiply(world, tmp, world);

			GLMatrixTranslation(tmp, obj.position[0], obj.position[1], obj.position[2]);
			GLMatrixMultiply(world, world, tmp);

			collect->SetMatrix("matWorld", world);
			collect->SetVector("matAmbient", &obj.color.r);
			collect->CommitChanges();

			if( obj.type == 0 )
				box->DrawSubset(0);
			else if( obj.type == 1 )
				dragon->DrawSubset(0);
		}
	}
	collect->End();

	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, 0);

	// STEP 3: render
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_ONE, GL_SRC_ALPHA);

	render->SetInt("screenWidth", screenwidth);

	render->Begin();
	{
		screenquad->Draw();
	}
	render->End();

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

	glDisable(GL_BLEND);

#ifdef _DEBUG
	// check errors
	GLenum err = glGetError();

	if( err != GL_NO_ERROR )
		std::cout << "Error\n";
#endif

	SwapBuffers(hdc);
}
//*************************************************************************************************************
