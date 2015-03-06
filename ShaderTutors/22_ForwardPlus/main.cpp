//*************************************************************************************************************
#include <Windows.h>
#include <iostream>

#include "../common/glext.h"

// TODO:
// - padlora normalmap
// - timer
// - include
// - r * r-el kell osztani attenben

// helper macros
#define TITLE				"Shader sample 22.1: Forward+ renderer"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_DELETE(x)		if( (x) ) { delete (x); (x) = 0; }

#define NUM_LIGHTS			400			// must be square number
#define LIGHT_RADIUS		1.5f		// must be at least 1
#define SHADOWMAP_SIZE		1024
#define DELAY				5
#define M_PI				3.141592f
#define M_2PI				6.283185f

// external variables
extern HWND		hwnd;
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
	float angle;
};

// sample variables
OpenGLMesh*			box				= 0;
OpenGLMesh*			teapot			= 0;
OpenGLEffect*		lightcull		= 0;
OpenGLEffect*		lightaccum		= 0;
OpenGLEffect*		ambient			= 0;
OpenGLEffect*		gammacorrect	= 0;
OpenGLEffect*		basic2D			= 0;
OpenGLEffect*		shadowedlight	= 0;
OpenGLEffect*		varianceshadow	= 0;
OpenGLEffect*		boxblur3x3		= 0;
OpenGLFramebuffer*	framebuffer		= 0;
OpenGLFramebuffer*	shadowmap		= 0;
OpenGLFramebuffer*	blurredshadow	= 0;
OpenGLScreenQuad*	screenquad		= 0;
OpenGLAABox			scenebox;

GLuint				texture1		= 0;
GLuint				texture2		= 0;
GLuint				texture3		= 0;
GLuint				headbuffer		= 0;	// head of linked lists
GLuint				nodebuffer		= 0;	// nodes of linked lists
GLuint				lightbuffer		= 0;	// light particles
GLuint				counterbuffer	= 0;	// atomic counter
GLuint				workgroupsx		= 0;
GLuint				workgroupsy		= 0;
int					timeout			= 0;
bool				hascompute		= false;

array_state<float, 2> cameraangle;

SceneObject objects[] =
{
	{ 0, { 0, -0.35f, 0 }, { 15, 0.5f, 15 }, 0 },

	{ 1, { -3 - 0.108f, -0.108f, -3 }, { 1, 1, 1 }, M_PI / -4 },
	{ 1, { -3 - 0.108f, -0.108f, 0 }, { 1, 1, 1 }, M_PI / -4 },
	{ 1, { -3 - 0.108f, -0.108f, 3 }, { 1, 1, 1 }, M_PI / -4 },

	{ 1, { -0.108f, -0.108f, -3 }, { 1, 1, 1 }, M_PI / 4 },
	{ 1, { -0.108f, -0.108f, 0 }, { 1, 1, 1 }, M_PI / 4 },
	{ 1, { -0.108f, -0.108f, 3 }, { 1, 1, 1 }, M_PI / 4 },

	{ 1, { 3 - 0.108f, -0.108f, -3 }, { 1, 1, 1 }, M_PI / -4 },
	{ 1, { 3 - 0.108f, -0.108f, 0 }, { 1, 1, 1 }, M_PI / -4 },
	{ 1, { 3 - 0.108f, -0.108f, 3 }, { 1, 1, 1 }, M_PI / -4 }
};

const int numobjects = sizeof(objects) / sizeof(SceneObject);

// sample functions
void UpdateParticles(float dt, bool generate);
void RenderScene(OpenGLEffect* effect);

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

	hascompute = (Quadron::qGLExtensions::ARB_compute_shader && Quadron::qGLExtensions::ARB_shader_storage_buffer_object);

#ifdef _DEBUG
	if( Quadron::qGLExtensions::ARB_debug_output )
	{
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
		glDebugMessageCallback(ReportGLError, 0);
	}
#endif

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	OpenGLMaterial* materials = 0;
	GLuint nummaterials = 0;

	// load objects
	if( !GLCreateMeshFromQM("../media/meshes/teapot.qm", &materials, &nummaterials, &teapot) )
	{
		MYERROR("Could not load teapot");
		return false;
	}

	delete[] materials;

	if( !GLCreateMeshFromQM("../media/meshes/cube.qm", &materials, &nummaterials, &box) )
	{
		MYERROR("Could not load box");
		return false;
	}

	delete[] materials;

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
			tmpbox = teapot->GetBoundingBox();

		tmpbox.TransformAxisAligned(world);

		scenebox.Add(tmpbox.Min);
		scenebox.Add(tmpbox.Max);
	}

	// create render targets
	framebuffer = new OpenGLFramebuffer(screenwidth, screenheight);
	framebuffer->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F);
	framebuffer->AttachTexture(GL_DEPTH_ATTACHMENT, GLFMT_D32F);
	
	if( !framebuffer->Validate() )
		return false;

	shadowmap = new OpenGLFramebuffer(SHADOWMAP_SIZE, SHADOWMAP_SIZE);
	shadowmap->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_G32R32F, GL_LINEAR);
	shadowmap->AttachRenderbuffer(GL_DEPTH_ATTACHMENT, GLFMT_D24S8);

	if( !shadowmap->Validate() )
		return false;

	blurredshadow = new OpenGLFramebuffer(SHADOWMAP_SIZE, SHADOWMAP_SIZE);
	blurredshadow->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_G32R32F, GL_LINEAR);

	if( !blurredshadow->Validate() )
		return false;

	screenquad = new OpenGLScreenQuad();

	// textures
	if( !GLCreateTextureFromFile("../media/textures/wood2.jpg", true, &texture1) )
	{
		MYERROR("Could not load texture");
		return false;
	}

	if( !GLCreateTextureFromFile("../media/textures/marble2.png", true, &texture2) )
	{
		MYERROR("Could not load texture");
		return false;
	}

	if( !GLCreateTextureFromFile("../media/textures/static_sky.jpg", true, &texture3) )
	{
		MYERROR("Could not load texture");
		return false;
	}

	// create buffers
	workgroupsx = (screenwidth + (screenwidth % 16)) / 16;
	workgroupsy = (screenheight + (screenheight % 16)) / 16;

	size_t numtiles = workgroupsx * workgroupsy;
	size_t headsize = 16;	// start, count, pad, pad
	size_t nodesize = 16;	// light index, next, pad, pad

	if( hascompute )
	{
		glGenBuffers(1, &headbuffer);
		glGenBuffers(1, &nodebuffer);
		glGenBuffers(1, &lightbuffer);
		glGenBuffers(1, &counterbuffer);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, headbuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numtiles * headsize, 0, GL_STATIC_DRAW);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodebuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, numtiles * nodesize * 1024, 0, GL_STATIC_DRAW);	// 4 MB

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightbuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_LIGHTS * sizeof(LightParticle), 0, GL_DYNAMIC_DRAW);

		UpdateParticles(0, true);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterbuffer);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), 0, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	}

	// load effects
	if( !GLCreateEffectFromFile("../media/shadersGL/basic2D.vert", 0, "../media/shadersGL/basic2D.frag", &basic2D) )
	{
		MYERROR("Could not load basic 2D shader");
		return false;
	}

	if( !GLCreateEffectFromFile("../media/shadersGL/basic2D.vert", 0, "../media/shadersGL/boxblur3x3.frag", &boxblur3x3) )
	{
		MYERROR("Could not load blur shader");
		return false;
	}

	if( !GLCreateEffectFromFile("../media/shadersGL/basic2D.vert", 0, "../media/shadersGL/gammacorrect.frag", &gammacorrect) )
	{
		MYERROR("Could not load gamma correction shader");
		return false;
	}

	if( !GLCreateEffectFromFile("../media/shadersGL/shadowmap_variance.vert", 0, "../media/shadersGL/shadowmap_variance.frag", &varianceshadow) )
	{
		MYERROR("Could not load shadowmap shader");
		return false;
	}

	if( !GLCreateEffectFromFile("../media/shadersGL/blinnphong_variance.vert", 0, "../media/shadersGL/blinnphong_variance.frag", &shadowedlight) )
	{
		MYERROR("Could not load shadowed light shader");
		return false;
	}

	if( !GLCreateEffectFromFile("../media/shadersGL/ambient.vert", 0, "../media/shadersGL/ambient.frag", &ambient) )
	{
		MYERROR("Could not load ambient shader");
		return false;
	}
	
	if( hascompute )
	{
		// light accumulation shader
		if( !GLCreateEffectFromFile("../media/shadersGL/lightaccum.vert", 0, "../media/shadersGL/lightaccum.frag", &lightaccum) )
		{
			MYERROR("Could not load light accumulation shader");
			return false;
		}

		// light culling shader
		if( !GLCreateComputeProgramFromFile("../media/shadersGL/lightcull.comp", 0, &lightcull) )
		{
			MYERROR("Could not load light culling shader");
			return false;
		}
	}

	float angles[2] = { -0.25f, 0.7f };
	cameraangle = angles;

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_DELETE(lightcull);
	SAFE_DELETE(lightaccum);
	SAFE_DELETE(ambient);
	SAFE_DELETE(gammacorrect);
	SAFE_DELETE(basic2D);
	SAFE_DELETE(shadowedlight);
	SAFE_DELETE(varianceshadow);
	SAFE_DELETE(boxblur3x3);
	SAFE_DELETE(teapot);
	SAFE_DELETE(box);
	SAFE_DELETE(framebuffer);
	SAFE_DELETE(shadowmap);
	SAFE_DELETE(blurredshadow);
	SAFE_DELETE(screenquad);

	if( texture1 )
		glDeleteTextures(1, &texture1);

	if( texture2 )
		glDeleteTextures(1, &texture2);

	if( texture3 )
		glDeleteTextures(1, &texture3);

	if( headbuffer )
		glDeleteBuffers(1, &headbuffer);

	if( nodebuffer )
		glDeleteBuffers(1, &nodebuffer);

	if( lightbuffer )
		glDeleteBuffers(1, &lightbuffer);

	if( counterbuffer )
		glDeleteBuffers(1, &counterbuffer);

	texture1		= 0;
	texture2		= 0;
	texture3		= 0;
	headbuffer		= 0;
	nodebuffer		= 0;
	lightbuffer		= 0;
	counterbuffer	= 0;

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
void UpdateParticles(float dt, bool generate)
{
	// NOTE: runs on 10 fps

	OpenGLAABox tmpbox = scenebox;
	float center[3];

	if( lightbuffer == 0 )
		return;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightbuffer);
	LightParticle* particles = (LightParticle*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	tmpbox.GetCenter(center);

	if( generate )
	{
		int segments = isqrt(NUM_LIGHTS);
		float theta, phi;

		OpenGLColor randomcolors[3] =
		{
			OpenGLColor(1, 0, 0, 1),
			OpenGLColor(0, 1, 0, 1),
			OpenGLColor(0, 0, 1, 1)
		};

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

				p.radius = LIGHT_RADIUS;
				p.color = randomcolors[(i + j) % 3];
			}
		}
	}
	else
	{
		float vx[3], vy[3], vz[3];
		float b[3];
		float A[16], Ainv[16];
		float planes[6][4];
		float denom, energy;
		float toi, besttoi;
		float impulse, noise;
		float (*bestplane)[4];
		bool pastcollision;

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

			b[0] = p.current[0] - p.previous[0];
			b[1] = p.current[1] - p.previous[1];
			b[2] = p.current[2] - p.previous[2];

			for( int j = 0; j < 6; ++j )
			{
				// use radius == 0.5
				denom = GLVec3Dot(b, planes[j]);
				pastcollision = (GLVec3Dot(p.previous, planes[j]) + planes[j][3] < 0.5f);

				if( denom < -1e-4f )
				{
					toi = (0.5f - GLVec3Dot(p.previous, planes[j]) - planes[j][3]) / denom;

					if( ((toi <= 1 && toi >= 0) ||		// normal case
						(toi < 0 && pastcollision)) &&	// allow past collision
						toi < besttoi )
					{
						besttoi = toi;
						bestplane = &planes[j];
					}
				}
			}

			if( besttoi <= 1 )
			{
				// resolve constraint
				p.current[0] = (1 - besttoi) * p.previous[0] + besttoi * p.current[0];
				p.current[1] = (1 - besttoi) * p.previous[1] + besttoi * p.current[1];
				p.current[2] = (1 - besttoi) * p.previous[2] + besttoi * p.current[2];

				impulse = -GLVec3Dot(*bestplane, p.velocity);

				// perturb normal vector
				noise = ((rand() % 100) / 100.0f) * M_PI * 0.333333f - M_PI * 0.166666f; // [-pi/6, pi/6]

				b[0] = cosf(noise + M_PI * 0.5f);
				b[1] = cosf(noise);
				b[2] = 0;

				GLVec3Normalize(vy, (*bestplane));
				GLGetOrthogonalVectors(vx, vz, vy);

				A[0] = vx[0];	A[1] = vy[0];	A[2] = vz[0];	A[3] = 0;
				A[4] = vx[1];	A[5] = vy[1];	A[6] = vz[1];	A[7] = 0;
				A[8] = vx[2];	A[9] = vy[2];	A[10] = vz[2];	A[11] = 0;
				A[12] = 0;		A[13] = 0;		A[14] = 0;		A[15] = 1;

				GLMatrixInverse(Ainv, A);
				GLVec3Transform(vy, b, Ainv);

				energy = GLVec3Length(p.velocity);

				p.velocity[0] += 2 * impulse * vy[0];
				p.velocity[1] += 2 * impulse * vy[1];
				p.velocity[2] += 2 * impulse * vy[2];

				// must conserve energy
				GLVec3Normalize(p.velocity, p.velocity);

				p.velocity[0] *= energy;
				p.velocity[1] *= energy;
				p.velocity[2] *= energy;
			}

#ifdef _DEBUG
			// test if a light fell through
			tmpbox.GetCenter(center);

			if( GLVec3Distance(p.current, center) > tmpbox.Radius() )
				::_CrtDbgBreak();
#endif
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

	if( timeout > DELAY )
		UpdateParticles(delta, false);
	else
		++timeout;
}
//*************************************************************************************************************
void RenderScene(OpenGLEffect* effect)
{
	float world[16];
	float worldinv[16];
	float tmp[16];

	GLMatrixIdentity(world);

	for( int i = 0; i < numobjects; ++i )
	{
		const SceneObject& obj = objects[i];

		GLMatrixScaling(tmp, obj.scale[0], obj.scale[1], obj.scale[2]);
		GLMatrixRotationAxis(world, obj.angle, 0, 1, 0);
		GLMatrixMultiply(world, tmp, world);

		GLMatrixTranslation(tmp, obj.position[0], obj.position[1], obj.position[2]);
		GLMatrixMultiply(world, world, tmp);

		GLMatrixInverse(worldinv, world);

		effect->SetMatrix("matWorld", world);
		effect->SetMatrix("matWorldInv", worldinv);

		if( obj.type == 0 )
		{
			float uv[] = { 2, 2, 0, 1 };

			effect->SetVector("uv", uv);
			effect->CommitChanges();

			glBindTexture(GL_TEXTURE_2D, texture1);
			box->DrawSubset(0);
		}
		else if( obj.type == 1 )
		{
			float uv[] = { 1, 1, 0, 1 };

			effect->SetVector("uv", uv);
			effect->CommitChanges();

			glBindTexture(GL_TEXTURE_2D, texture2);
			teapot->DrawSubset(0);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	float tmp[16];
	float texmat[16];
	float view[16];
	float viewinv[16];
	float proj[16];
	float viewproj[16];
	float lightview[16];
	float lightproj[16];
	float lightviewproj[16];

	float globalambient[4]	= { 0.01f, 0.01f, 0.01f, 1.0f };
	float moonlight[]		= { -0.25f, 0.65f, -1, 0 };
	float mooncolor[]		= { 0.6f, 0.6f, 1, 1 };

	float eye[3]			= { 0, 0, 8 };
	float look[3]			= { 0, 0, 0 };
	float up[3]				= { 0, 1, 0 };

	float screensize[2]		= { (float)screenwidth, (float)screenheight };
	float clipplanes[2];
	float orient[2];

	// setup camera
	cameraangle.smooth(orient, alpha);

	GLMatrixRotationYawPitchRoll(view, orient[0], orient[1], 0);
	GLVec3Transform(eye, eye, view);

	GLFitToBox(clipplanes[0], clipplanes[1], eye, look, scenebox);
	GLMatrixPerspectiveRH(proj, (60.0f * 3.14159f) / 180.f,  (float)screenwidth / (float)screenheight, clipplanes[0], clipplanes[1]);

	GLMatrixLookAtRH(view, eye, look, up);
	GLMatrixMultiply(viewproj, view, proj);

	// setup moonlight
	GLMatrixInverse(viewinv, view);
	GLVec3Transform(moonlight, moonlight, viewinv);
	GLVec3Normalize(moonlight, moonlight);

	// should be that value in view space (background is fix)
	// but let y stay in world space, so we see shadow
	moonlight[1] = 0.65f;

	GLMatrixViewVector(lightview, moonlight);
	GLFitToBox(lightproj, lightview, scenebox);
	GLMatrixMultiply(lightviewproj, lightview, lightproj);

	// render shadow map
	glClearColor(0, 0, 0, 1);

	varianceshadow->SetMatrix("matViewProj", lightviewproj);

	shadowmap->Set();
	{
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		varianceshadow->Begin();
		{
			RenderScene(varianceshadow);
		}
		varianceshadow->End();
	}
	shadowmap->Unset();

	// blur it
	float texelsize[] = { 1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE };

	glDepthMask(GL_FALSE);
	glBindTexture(GL_TEXTURE_2D, shadowmap->GetColorAttachment(0));

	GLMatrixIdentity(texmat);

	boxblur3x3->SetMatrix("matTexture", texmat);
	boxblur3x3->SetMatrix("texelSize", texelsize);
	boxblur3x3->SetInt("sampler0", 0);

	blurredshadow->Set();
	{
		boxblur3x3->Begin();
		{
			screenquad->Draw();
		}
		boxblur3x3->End();
	}
	blurredshadow->Unset();

	glDepthMask(GL_TRUE);

	// STEP 1: z pass
	ambient->SetMatrix("matViewProj", viewproj);
	ambient->SetVector("matAmbient", globalambient);

	framebuffer->Set();
	{
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		// draw background first
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glBindTexture(GL_TEXTURE_2D, texture3);

		float scaledx = 1360.0f * (screenheight / 768.0f);
		float scale = screenwidth / scaledx;

		GLMatrixTranslation(tmp, -0.5f, 0, 0);
		GLMatrixScaling(texmat, scale, 1, 1);
		GLMatrixMultiply(texmat, tmp, texmat);

		GLMatrixTranslation(tmp, 0.5f, 0, 0);
		GLMatrixMultiply(texmat, texmat, tmp);

		GLMatrixRotationAxis(tmp, M_PI, 0, 0, 1);
		GLMatrixMultiply(texmat, texmat, tmp);

		basic2D->SetInt("sampler0", 0);
		basic2D->SetMatrix("matTexture", texmat);
		basic2D->Begin();
		{
			screenquad->Draw();
		}
		basic2D->End();

		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);

		// then fill zbuffer
		ambient->Begin();
		{
			RenderScene(ambient);
		}
		ambient->End();
	}
	framebuffer->Unset();

	// STEP 2: cull lights
	if( lightcull && timeout > DELAY )
	{
		lightcull->SetInt("depthSampler", 0);
		lightcull->SetInt("numLights", NUM_LIGHTS);
		lightcull->SetFloat("alpha", alpha);
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
	}

	// STEP 3: add some moonlight with shadow
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDepthMask(GL_FALSE);

	framebuffer->Set();

	shadowedlight->SetMatrix("matViewProj", viewproj);
	shadowedlight->SetMatrix("lightViewProj", lightviewproj);
	shadowedlight->SetVector("eyePos", eye);
	shadowedlight->SetVector("lightPos", moonlight);
	shadowedlight->SetVector("lightColor", mooncolor);
	shadowedlight->SetInt("sampler0", 0);
	shadowedlight->SetInt("sampler1", 1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, blurredshadow->GetColorAttachment(0));
	glActiveTexture(GL_TEXTURE0);

	shadowedlight->Begin();
	{
		RenderScene(shadowedlight);
	}
	shadowedlight->End();

	// STEP 4: accumulate lighting
	if( lightaccum && timeout > DELAY )
	{
		lightaccum->SetMatrix("matViewProj", viewproj);
		lightaccum->SetVector("eyePos", eye);
		lightaccum->SetFloat("alpha", alpha);
		lightaccum->SetInt("sampler0", 0);
		lightaccum->SetInt("numTilesX", workgroupsx);

		lightaccum->Begin();
		{
			RenderScene(lightaccum);
		}
		lightaccum->End();
		
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
	}

	framebuffer->Unset();

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	// STEP 4: gamma correct
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, framebuffer->GetColorAttachment(0));

	GLMatrixIdentity(texmat);

	gammacorrect->SetInt("sampler0", 0);
	gammacorrect->SetMatrix("matTexture", texmat);
	gammacorrect->Begin();
	{
		screenquad->Draw();
	}
	gammacorrect->End();

#ifdef _DEBUG
	// check errors
	GLenum err = glGetError();

	if( err != GL_NO_ERROR )
		std::cout << "Error\n";
#endif

	SwapBuffers(hdc);
}
//*************************************************************************************************************
