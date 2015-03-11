
#include "win32window.h"
#include "renderingcore.h"
#include "drawingitem.h"

class OpenGLAddonTask : public RenderingCore::IRenderingTask
{
	enum AddonAction
	{
		AA_Setup = 0,
		AA_Render = 1
	};

private:
	OpenGLColor			clearcolor;
	OpenGLColor			meshcolor;
	OpenGLMesh*			mesh;
	OpenGLEffect*		effect;
	OpenGLFramebuffer*	rendertarget;	// external
	float				meshoffset[3];
	float				rendertime;
	int					action;
	bool				cleardepth;

	void Dispose();
	void Execute(IRenderingContext* context);
	void Internal_Render();

public:
	OpenGLAddonTask(int universe, OpenGLFramebuffer* target);

	void Setup();
	void Render(float time);

	inline void SetClearOptions(const OpenGLColor& color, bool depth) {
		clearcolor = color;
		cleardepth = depth;
	}

	inline void SetMeshColor(const OpenGLColor& color) {
		meshcolor = color;
	}

	inline void SetMeshOffset(float x, float y, float z) {
		GLVec3Set(meshoffset, x, y, z);
	}

	inline void SetRenderTarget(OpenGLFramebuffer* target) {
		rendertarget = target;
	}
};

class RenderTargetBlitTask : public RenderingCore::IRenderingTask
{
private:
	OpenGLFramebuffer*	source;
	OpenGLFramebuffer*	target;
	unsigned int		blitflags;

	void Dispose()
	{
	}

	void Execute(IRenderingContext* context)
	{
		context->Blit(source, target, blitflags);
	}

public:
	enum BlitFlags
	{
		Color = GL_COLOR_BUFFER_BIT,
		Depth = GL_DEPTH_BUFFER_BIT
	};

	RenderTargetBlitTask(int universe, OpenGLFramebuffer* from, OpenGLFramebuffer* to, unsigned int flags)
		: IRenderingTask(universe)
	{
		source = from;
		target = to;
		blitflags = flags;
	}
};

// *****************************************************************************************************************************
//
// OpenGLAddonTask impl
//
// *****************************************************************************************************************************

OpenGLAddonTask::OpenGLAddonTask(int universe, OpenGLFramebuffer* target)
	: IRenderingTask(universe)
{
	mesh			= 0;
	effect			= 0;
	clearcolor		= OpenGLColor(1, 1, 1, 1);
	meshcolor		= OpenGLColor(1, 1, 1, 1);
	rendertarget	= target;
	action			= AA_Setup;
	rendertime		= 0;
	cleardepth		= true;

	GLVec3Set(meshoffset, 0, 0, 0);
}

void OpenGLAddonTask::Dispose()
{
	// NOTE: runs on renderer thread
	SAFE_DELETE(mesh);
	SAFE_DELETE(effect);

	rendertarget = 0;
}

void OpenGLAddonTask::Execute(IRenderingContext* context)
{
	// NOTE: runs on renderer thread
	if( action == AA_Setup )
	{
		if( !mesh )
			mesh = context->CreateMesh("../media/meshes/teapot.qm");

		if( !effect )
			effect = context->CreateEffect("../media/shadersGL/blinnphong.vert", 0, "../media/shadersGL/blinnphong.frag");
	}
	else if( action == AA_Render )
		Internal_Render();
}

void OpenGLAddonTask::Internal_Render()
{
	// NOTE: runs on renderer thread
	if( !rendertarget )
		return;

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

	GLMatrixLookAtRH(view, eye, look, up);
	GLMatrixPerspectiveRH(
		proj, (60.0f * 3.14159f) / 180.f, 
		(float)rendertarget->GetWidth() / (float)rendertarget->GetHeight(), 0.1f, 100.0f);
	
	GLMatrixMultiply(viewproj, view, proj);

	//calculate world matrix
	GLMatrixIdentity(tmp2);
	
	tmp2[12] = -0.108f;		// offset with bb center
	tmp2[13] = -0.7875f;	// offset with bb center

	GLMatrixRotationAxis(tmp1, fmodf(rendertime * 20.0f, 360.0f) * (3.14152f / 180.0f), 1, 0, 0);
	GLMatrixMultiply(world, tmp2, tmp1);

	GLMatrixRotationAxis(tmp2, fmodf(rendertime * 20.0f, 360.0f) * (3.14152f / 180.0f), 0, 1, 0);
	GLMatrixMultiply(world, world, tmp2);

	world[12] += meshoffset[0];
	world[13] += meshoffset[1];
	world[14] += meshoffset[2];

	// setup states (TODO:)
	glClearColor(clearcolor.r, clearcolor.g, clearcolor.b, clearcolor.a);
	glClearDepth(1.0f);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	// render
	effect->SetMatrix("matWorld", world);
	effect->SetMatrix("matViewProj", viewproj);

	effect->SetVector("color", &meshcolor.r);
	effect->SetVector("eyePos", eye);
	effect->SetVector("lightPos", lightpos);

	rendertarget->Set();
	{
		if( cleardepth )
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		else
			glClear(GL_COLOR_BUFFER_BIT);

		effect->Begin();
		{
			mesh->DrawSubset(0);
		}
		effect->End();
	}
	rendertarget->Unset();

	// check errors
	GLenum err = glGetError();

	if( err != GL_NO_ERROR )
		std::cout << "GL error\n";
}

void OpenGLAddonTask::Setup()
{
	// NOTE: runs on any other thread
	action = AA_Setup;

	GetRenderingCore()->AddTask(this);
	Wait();
}

void OpenGLAddonTask::Render(float time)
{
	// NOTE: runs on any other thread
	action = AA_Render;
	rendertime = time;

	GetRenderingCore()->AddTask(this);
}

// *****************************************************************************************************************************
//
// Main window functions
//
// *****************************************************************************************************************************

OpenGLAddonTask* addonrenderer = 0;

void MainWindow_Created(Win32Window* window)
{
	DrawingItem* drawingitem = window->GetDrawingItem();
	const DrawingLayer& bottomlayer = drawingitem->GetBottomLayer();

	if( !addonrenderer )
	{
		addonrenderer = new OpenGLAddonTask(drawingitem->GetOpenGLUniverseID(), bottomlayer.GetRenderTarget());
		addonrenderer->Setup();
	}
}

void MainWindow_Closing(Win32Window*)
{
	if( addonrenderer )
	{
		addonrenderer->Release();
		addonrenderer = 0;
	}
}

void MainWindow_KeyPress(Win32Window* window, WPARAM wparam)
{
	switch( wparam )
	{
	case VK_ESCAPE:
		window->Close();
		break;

	default:
		break;
	}
}

void MainWindow_Render(Win32Window* window, float alpha, float elapsedtime)
{
	static float time = 0;

	DrawingItem* drawingitem = window->GetDrawingItem();
	time += elapsedtime;

	if( addonrenderer )
	{
		addonrenderer->SetClearOptions(OpenGLColor(0.0f, 0.125f, 0.3f, 1.0f), true);
		addonrenderer->Render(time);
	}

	drawingitem->RecomposeLayers();
}

// *****************************************************************************************************************************
//
// Window 3 functions
//
// *****************************************************************************************************************************

OpenGLAddonTask* window3renderer = 0;

void Window3_Created(Win32Window* window)
{
	DrawingItem* drawingitem = window->GetDrawingItem();
	const DrawingLayer& bottomlayer = drawingitem->GetBottomLayer();

	if( !window3renderer )
	{
		window3renderer = new OpenGLAddonTask(drawingitem->GetOpenGLUniverseID(), bottomlayer.GetRenderTarget());
		window3renderer->Setup();
	}
}

void Window3_Closing(Win32Window*)
{
	if( window3renderer )
	{
		window3renderer->Release();
		window3renderer = 0;
	}
}

void Window3_Render(Win32Window* window, float alpha, float elapsedtime)
{
	static float time = 0;

	DrawingItem* drawingitem = window->GetDrawingItem();
	time += elapsedtime;

	if( window3renderer )
	{
		OpenGLFramebuffer* bottomlayer = drawingitem->GetBottomLayer().GetRenderTarget();
		OpenGLFramebuffer* feedbacklayer = drawingitem->GetFeedbackLayer().GetRenderTarget();

		window3renderer->SetRenderTarget(bottomlayer);
		window3renderer->SetClearOptions(OpenGLColor(0.0f, 0.125f, 0.3f, 1.0f), true);
		window3renderer->SetMeshColor(OpenGLColor(1, 1, 1, 1));
		window3renderer->SetMeshOffset(-0.75f, 0, 0);
		window3renderer->Render(time);
		window3renderer->Wait();

		RenderTargetBlitTask* blitter = new RenderTargetBlitTask(
			drawingitem->GetOpenGLUniverseID(), bottomlayer, feedbacklayer, RenderTargetBlitTask::Depth);

		GetRenderingCore()->AddTask(blitter);
		blitter->Release();

		window3renderer->SetRenderTarget(feedbacklayer);
		window3renderer->SetClearOptions(OpenGLColor(0, 0, 0, 0), false);
		window3renderer->SetMeshColor(OpenGLColor(0, 1, 0, 1));
		window3renderer->SetMeshOffset(0.75f, 0, 0);
		window3renderer->Render(-time);
	}

	drawingitem->RecomposeLayers();
}
