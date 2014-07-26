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

#define NUM_LIGHTS			256			// must be square number
#define M_PI				3.141592f
#define M_2PI				6.283185f

// external variables
extern HDC		hdc;
extern long		screenwidth;
extern long		screenheight;
extern short	mousedx;
extern short	mousedy;
extern short	mousedown;

// sample structures
struct LightParticle
{
	OpenGLColor	color;
	float		previous[4];
	float		current[4];
	float		velocity[3];
	float		radius;
};

struct SceneObject
{
	int type;			// 0 for box, 1 for teapot
	float position[3];
	float scale[3];
};

// sample variables
OpenGLMesh*			box				= 0;
OpenGLMesh*			teapot			= 0;
OpenGLEffect*		lightcull		= 0;
OpenGLEffect*		lightaccum		= 0;
OpenGLEffect*		ambient			= 0;
OpenGLEffect*		basic2D			= 0;
OpenGLFramebuffer*	framebuffer		= 0;
OpenGLScreenQuad*	screenquad		= 0;
OpenGLAABox			scenebox;

array_state<float, 2>	cameraangle;

GLuint				texture			= 0;
GLuint				headbuffer		= 0;	// head of linked lists
GLuint				nodebuffer		= 0;	// nodes of linked lists
GLuint				lightbuffer		= 0;
GLuint				counterbuffer	= 0;	// atomic counter
GLuint				workgroupsx		= 0;
GLuint				workgroupsy		= 0;

SceneObject objects[] =
{
	{ 0, { 0, -1, 0 }, { 20, 0.5f, 20 } },

	{ 1, { -3 - 0.108f, -0.108f, -3 }, { 1, 1, 1 } },
	{ 1, { -3 - 0.108f, -0.108f, 0 }, { 1, 1, 1 } },
	{ 1, { -3 - 0.108f, -0.108f, 3 }, { 1, 1, 1 } },

	{ 1, { -0.108f, -0.108f, -3 }, { 1, 1, 1 } },
	{ 1, { -0.108f, -0.108f, 0 }, { 1, 1, 1 } },
	{ 1, { -0.108f, -0.108f, 3 }, { 1, 1, 1 } },

	{ 1, { 3 - 0.108f, -0.108f, -3 }, { 1, 1, 1 } },
	{ 1, { 3 - 0.108f, -0.108f, 0 }, { 1, 1, 1 } },
	{ 1, { 3 - 0.108f, -0.108f, 3 }, { 1, 1, 1 } }
};

const int numobjects = sizeof(objects) / sizeof(SceneObject);

// sample functions
void UpdateParticles(float dt, bool generate);
void RenderScene(OpenGLEffect* effect);

void APIENTRY ReportGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userdata)
{
	if( source == GL_DEBUG_SOURCE_API )
		std::cout << "GL(" << severity << "): ";
	else if( source == GL_DEBUG_SOURCE_SHADER_COMPILER )
		std::cout << "GLSL(" << severity << "): ";
	else
		std::cout << "OTHER(" << severity << "): ";

	std::cout << id << ": " << message << "\n";
}

bool InitScene()
{
	Quadron::qGLExtensions::QueryFeatures(hdc);

#ifdef _DEBUG
	if( Quadron::qGLExtensions::ARB_debug_output )
	{
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
		glDebugMessageCallback(ReportGLError, 0);
	}
#endif

	/*
	GLint maxgroups[3];

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxgroups[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxgroups[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxgroups[2]);
	*/

	//glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearColor(0.0f, 0.0103f, 0.0707f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	OpenGLMaterial* materials = 0;
	GLuint nummaterials = 0;

	// load objects
	if( !GLLoadMeshFromQM("../media/meshes/teapot.qm", &materials, &nummaterials, &teapot) )
	{
		MYERROR("Could not load teapot");
		return false;
	}

	delete[] materials;

	if( !GLLoadMeshFromQM("../media/meshes/cube.qm", &materials, &nummaterials, &box) )
	{
		MYERROR("Could not load box");
		return false;
	}

	delete[] materials;

	// calculate scene bounding box
	OpenGLAABox tmpbox;
	float world[16];

	GLMatrixIdentity(world);

	for( int i = 0; i < numobjects; ++i )
	{
		const SceneObject& obj = objects[i];

		world[0] = obj.scale[0];
		world[5] = obj.scale[1];
		world[10] = obj.scale[2];

		world[12] = obj.position[0];
		world[13] = obj.position[1];
		world[14] = obj.position[2];

		if( obj.type == 0 )
			tmpbox = box->GetBoundingBox();
		else if( obj.type == 1 )
			tmpbox = teapot->GetBoundingBox();

		tmpbox.TransformAxisAligned(world);

		scenebox.Add(tmpbox.Min);
		scenebox.Add(tmpbox.Max);
	}

	// create render targets
	framebuffer = new OpenGLFramebuffer(screenwidth, screenheight);
	framebuffer->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A8R8G8B8);
	framebuffer->AttachTexture(GL_DEPTH_ATTACHMENT, GLFMT_D32F);
	
	if( !framebuffer->Validate() )
		return false;

	screenquad = new OpenGLScreenQuad();

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
	glGenBuffers(1, &lightbuffer);
	glGenBuffers(1, &counterbuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, headbuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numtiles * headsize, 0, GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodebuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numtiles * nodesize * 1024, 0, GL_STATIC_DRAW);	// 2 MB

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightbuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_LIGHTS * sizeof(LightParticle), 0, GL_DYNAMIC_DRAW);

	UpdateParticles(0, true);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterbuffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), 0, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	if( !GLCreateEffectFromFile("../media/shadersGL/basic2D.vert", "../media/shadersGL/basic2D.frag", &basic2D) )
	{
		MYERROR("Could not load basic 2D shader");
		return false;
	}

	// light accumulation shader
	if( !GLCreateEffectFromFile("../media/shadersGL/lightaccum.vert", "../media/shadersGL/lightaccum.frag", &lightaccum) )
	{
		MYERROR("Could not load light accumulation shader");
		return false;
	}

	if( !GLCreateEffectFromFile("../media/shadersGL/ambientzpass.vert", "../media/shadersGL/ambientzpass.frag", &ambient) )
	{
		MYERROR("Could not load ambient shader");
		return false;
	}

	// light culling shader
	if( !GLCreateComputeProgramFromFile("../media/shadersGL/lightcull.comp", 0, &lightcull) )
	{
		MYERROR("Could not load light culling shader");
		return false;
	}

	float angles[2] = { -0.25f, 0.7f };
	cameraangle = angles;

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
	if( lightcull )
		delete lightcull;

	if( lightaccum )
		delete lightaccum;

	if( ambient )
		delete ambient;

	if( basic2D )
		delete basic2D;

	if( teapot )
		delete teapot;

	if( box )
		delete box;

	if( framebuffer )
		delete framebuffer;

	if( screenquad )
		delete screenquad;

	if( texture )
		glDeleteTextures(1, &texture);

	if( headbuffer )
		glDeleteBuffers(1, &headbuffer);

	if( nodebuffer )
		glDeleteBuffers(1, &nodebuffer);

	if( lightbuffer )
		glDeleteBuffers(1, &lightbuffer);

	if( counterbuffer )
		glDeleteBuffers(1, &counterbuffer);

	lightbuffer = 0;
	lightcull = 0;
	lightaccum = 0;
	teapot = 0;
	texture = 0;
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
}
//*************************************************************************************************************
void UpdateParticles(float dt, bool generate)
{
	OpenGLAABox tmpbox = scenebox;

	if( lightbuffer == 0 )
		return;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightbuffer);
	LightParticle* particles = (LightParticle*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	// make it bigger
	tmpbox.Max[1] += 1;

	if( generate )
	{
		int segments = isqrt(NUM_LIGHTS);
		float theta, phi;
		float center[3];

		OpenGLColor randomcolors[3] =
		{
			OpenGLColor(1, 0, 0, 1),
			OpenGLColor(0, 1, 0, 1),
			OpenGLColor(0, 0, 1, 1)
		};

		tmpbox.GetCenter(center);

		for( int i = 0; i < segments; ++i )
		{
			for( int j = 0; j < segments; ++j )
			{
				LightParticle& p = particles[i * segments + j];

				theta = ((float)j / (segments - 1)) * M_PI;
				phi = ((float)i / (segments - 1)) * M_2PI;

				p.previous[0] = center[0];
				p.previous[1] = center[1];
				p.previous[2] = center[2];
				p.previous[3] = 1;

				p.velocity[0] = sinf(theta) * cosf(phi) * 2;
				p.velocity[1] = cosf(theta) * 2;
				p.velocity[2] = sinf(theta) * sinf(phi) * 2;

				p.current[0] = p.previous[0];
				p.current[1] = p.previous[1];
				p.current[2] = p.previous[2];
				p.current[3] = 1;

				p.radius = 1; // must be at least 1
				p.color = randomcolors[(i + j) % 3];
			}
		}
	}
	else
	{
		float planes[6][4];
		float denom;
		float besttoi;
		float toi;
		float impulse;
		float (*bestplane)[4];
		bool closing;

		tmpbox.GetPlanes(planes);

		for( int i = 0; i < NUM_LIGHTS; ++i )
		{
			LightParticle& p = particles[i];

			// integrate
			p.previous[0] = p.current[0];
			p.previous[1] = p.current[1];
			p.previous[2] = p.current[2];

			p.current[0] += p.velocity[0] * dt;
			p.current[1] += p.velocity[1] * dt;
			p.current[2] += p.velocity[2] * dt;

			// detect collision
			besttoi = 2;

			for( int j = 0; j < 6; ++j )
			{
				closing = (GLVec3Dot(p.velocity, planes[j]) < 0);
				denom = GLVec3Dot(p.current, planes[j]) + planes[j][3];

				if( closing && fabs(denom) > 0 )
				{
					// p.radius
					toi = (1 - GLVec3Dot(p.previous, planes[j]) - planes[j][3]) / denom;

					if( toi <= 1 && toi >= 0 && toi < besttoi )
					{
						besttoi = toi;
						bestplane = &planes[j];
					}
				}
			}

			if( besttoi >= 0 && besttoi <= 1 )
			{
				// resolve constraint
				p.current[0] = (1 - besttoi) * p.previous[0] + besttoi * p.current[0];
				p.current[1] = (1 - besttoi) * p.previous[1] + besttoi * p.current[1];
				p.current[2] = (1 - besttoi) * p.previous[2] + besttoi * p.current[2];
			
				impulse = -GLVec3Dot(*bestplane, p.velocity);

				p.velocity[0] += 2 * impulse * (*bestplane)[0];
				p.velocity[1] += 2 * impulse * (*bestplane)[1];
				p.velocity[2] += 2 * impulse * (*bestplane)[2];
			}
		}
	}

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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

	UpdateParticles(delta, false);
}
//*************************************************************************************************************
void RenderScene(OpenGLEffect* effect)
{
	float world[16];

	GLMatrixIdentity(world);
	glBindTexture(GL_TEXTURE_2D, texture);

	for( int i = 0; i < numobjects; ++i )
	{
		const SceneObject& obj = objects[i];

		world[0] = obj.scale[0];
		world[5] = obj.scale[1];
		world[10] = obj.scale[2];

		world[12] = obj.position[0];
		world[13] = obj.position[1];
		world[14] = obj.position[2];

		effect->SetMatrix("matWorld", world);
		effect->CommitChanges();

		if( obj.type == 0 )
			box->DrawSubset(0);
		else if( obj.type == 1 )
			teapot->DrawSubset(0);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	float globalambient[4]	= { 0, 0, 0, 1.0f };
	float lightpos[4]		= { 6, 3, 10, 1 };
	float clipplanes[2];
	float screensize[2]		= { (float)screenwidth, (float)screenheight };
	float eye[3]			= { 0, 0, 8 };
	float look[3]			= { 0, 0, 0 };
	float up[3]				= { 0, 1, 0 };
	float orient[2];

	float view[16];
	float proj[16];
	float viewproj[16];

	cameraangle.smooth(orient, alpha);

	GLMatrixRotationYawPitchRoll(view, orient[0], orient[1], 0);
	GLVec3Transform(eye, eye, view);

	GLFitToBox(clipplanes[0], clipplanes[1], eye, look, scenebox);
	GLMatrixPerspectiveRH(proj, (60.0f * 3.14159f) / 180.f,  (float)screenwidth / (float)screenheight, clipplanes[0], clipplanes[1]);

	GLMatrixLookAtRH(view, eye, look, up);
	GLMatrixMultiply(viewproj, view, proj);
	
	time += elapsedtime;

	// STEP 1: z pass
	ambient->SetMatrix("matViewProj", viewproj);
	ambient->SetVector("matAmbient", globalambient);

	framebuffer->Set();
	{
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		ambient->Begin();
		{
			RenderScene(ambient);
		}
		ambient->End();
	}
	framebuffer->Unset();

	// STEP 2: cull lights
	lightcull->SetInt("depthSampler", 0);
	lightcull->SetInt("numLights", NUM_LIGHTS);
	lightcull->SetVector("clipPlanes", clipplanes);
	lightcull->SetVector("screenSize", screensize);
	lightcull->SetMatrix("matProj", proj);
	lightcull->SetMatrix("matView", view);
	lightcull->SetMatrix("matViewProj", viewproj);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterbuffer);
	GLuint* counter = (GLuint*)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
	
	*counter = 0;
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, headbuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, nodebuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lightbuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counterbuffer);
	glBindTexture(GL_TEXTURE_2D, framebuffer->GetDepthAttachment());

	lightcull->Begin();
	{
		glDispatchCompute(workgroupsx, workgroupsy, 1);
	}
	lightcull->End();

	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, 0);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// STEP 3: accumulate lighting
	lightaccum->SetMatrix("matViewProj", viewproj);
	lightaccum->SetVector("eyePos", eye);
	lightaccum->SetInt("sampler0", 0);

	framebuffer->Set();
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glDepthMask(GL_FALSE);

		lightaccum->Begin();
		{
			RenderScene(lightaccum);
		}
		lightaccum->End();

		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
	framebuffer->Unset();

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);

	// STEP 4: gamma correct
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, framebuffer->GetColorAttachment(0));

	basic2D->SetInt("sampler0", 0);
	basic2D->Begin();
	{
		screenquad->Draw();
	}
	basic2D->End();

	// check errors
	GLenum err = glGetError();

	if( err != GL_NO_ERROR )
		std::cout << "Error\n";

	SwapBuffers(hdc);
}
//*************************************************************************************************************
