
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
	OpenGLMesh*			mesh;
	OpenGLEffect*		effect;
	OpenGLFramebuffer*	rendertarget;	// external
	float				rendertime;
	int					action;

	void Dispose();
	void Execute(IRenderingContext* context);
	void Internal_Render();

public:
	OpenGLAddonTask(int universe, OpenGLFramebuffer* target);

	void Setup();
	void Render(float time);
};

OpenGLAddonTask::OpenGLAddonTask(int universe, OpenGLFramebuffer* target)
	: IRenderingTask(universe)
{
	mesh			= 0;
	effect			= 0;
	rendertarget	= target;
	action			= AA_Setup;
	rendertime		= 0;
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

	// setup states
	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0f);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	// render
	effect->SetMatrix("matWorld", world);
	effect->SetMatrix("matViewProj", viewproj);
	effect->SetVector("eyePos", eye);
	effect->SetVector("lightPos", lightpos);

	rendertarget->Set();
	{
		glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

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
		addonrenderer->Render(time);

	drawingitem->RecomposeLayers();
}
