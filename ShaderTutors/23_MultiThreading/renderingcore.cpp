
#include <iostream>
#include "renderingcore.h"

#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

typedef HGLRC (APIENTRY *WGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL (APIENTRY *WGLGETPIXELFORMATATTRIBIVARBPROC)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
typedef BOOL (APIENTRY *WGLGETPIXELFORMATATTRIBFVARBPROC)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
typedef BOOL (APIENTRY *WGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

WGLCREATECONTEXTATTRIBSARBPROC		wglCreateContextAttribsARB = 0;
WGLGETPIXELFORMATATTRIBIVARBPROC	wglGetPixelFormatAttribivARB = 0;
WGLGETPIXELFORMATATTRIBFVARBPROC	wglGetPixelFormatAttribfvARB = 0;
WGLCHOOSEPIXELFORMATARBPROC			wglChoosePixelFormatARB = 0;

static int		GLMajorVersion = 0;
static int		GLMinorVersion = 0;

RenderingCore*	RenderingCore::_inst = 0;
Guard			RenderingCore::singletonguard;

namespace Quadron
{
	extern bool wIsSupported(const char* name, HDC hdc);
}

RenderingCore* GetRenderingCore()
{
	RenderingCore::singletonguard.Lock();

	if( !RenderingCore::_inst )
		RenderingCore::_inst = new RenderingCore();

	RenderingCore::singletonguard.Unlock();

	return RenderingCore::_inst;
}

IRenderingContext::~IRenderingContext()
{
}

// *****************************************************************************************************************************
//
// Private interface for rendering core
//
// *****************************************************************************************************************************

class RenderingCore::PrivateInterface : public IRenderingContext
{
	friend class UniverseCreatorTask;
	friend class RenderingCore;

	struct OpenGLContext
	{
		HDC hdc;
		HGLRC hrc;

		OpenGLContext()
			: hdc(0), hrc(0)
		{
		}
	};

	typedef std::vector<OpenGLContext> contextlist;

private:
	contextlist		contexts;
	OpenGLContext	activecontext;

	// internal methods
	int CreateContext(HDC hdc);
	void DeleteContext(int id);
	bool ActivateContext(int id);
	HDC GetDC(int id) const;

public:
	// factory methods
	OpenGLFramebuffer*	CreateFramebuffer(GLuint width, GLuint height);
	OpenGLScreenQuad*	CreateScreenQuad();
	OpenGLEffect*		CreateEffect(const char* vsfile, const char* gsfile, const char* fsfile);
	OpenGLMesh*			CreateMesh(const char* file);
	OpenGLMesh*			CreateMesh(GLuint numvertices, GLuint numindices, GLuint flags, OpenGLVertexElement* decl);

	// rendering methods
	void Blit(OpenGLFramebuffer* from, OpenGLFramebuffer* to, GLbitfield flags);
	void Clear(const OpenGLColor& color);
	void Present(int id);
};

int RenderingCore::PrivateInterface::CreateContext(HDC hdc)
{
	if( !wglCreateContextAttribsARB )
		return -1;
	//	return CreateLegacyContext(hdc);

	OpenGLContext	context;
	int				attrib[128];
	int				i = 0;
	int				pixelformat;
	UINT			numformats;

	context.hrc = 0;
	context.hdc = 0;

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32, // color
		0, 0, 0, 0, 0, 0,
		0, // alpha
		0, 0, 0, 0, 0, 0,
		24, 8, 0, // depth, stencil, aux
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	memset(attrib, 0, sizeof(attrib));

	attrib[i++] = 0x2001;		// WGL_DRAW_TO_WINDOW_ARB;
	attrib[i++] = TRUE;
	attrib[i++] = 0x2003;		// WGL_ACCELERATION_ARB;
	attrib[i++] = 0x2027;		// WGL_FULL_ACCELERATION_ARB
	attrib[i++] = 0x2010;		// WGL_SUPPORT_OPENGL_ARB
	attrib[i++] = TRUE;
	attrib[i++] = 0x2011;		// WGL_DOUBLE_BUFFER_ARB
	attrib[i++] = TRUE;
	attrib[i++] = 0x2013;		// WGL_PIXEL_TYPE_ARB
	attrib[i++] = 0x202B;		// WGL_TYPE_RGBA_ARB
	attrib[i++] = 0x2014;		// WGL_COLOR_BITS_ARB
	attrib[i++] = 32;
	attrib[i++] = 0x201B;		// WGL_ALPHA_BITS_ARB
	attrib[i++] = 0;
	attrib[i++] = 0x2022;		// WGL_DEPTH_BITS_ARB
	attrib[i++] = 24;
	attrib[i++] = 0x2023;		// WGL_STENCIL_BITS_ARB
	attrib[i++] = 8;
	attrib[i++] = 0;

	wglChoosePixelFormatARB(hdc, attrib, 0, 1, &pixelformat, &numformats);

	if( numformats != 0 )
	{
		std::cout << "Selected pixel format: " << pixelformat <<"\n";

		int success = SetPixelFormat(hdc, pixelformat, &pfd);
		V_RETURN(-1, "PrivateInterface::CreateContext(): Could not setup pixel format", success);

		int contextattribs[] =
		{
			0x2091,		// WGL_CONTEXT_MAJOR_VERSION_ARB
			GLMajorVersion,
			0x2092,		// WGL_CONTEXT_MINOR_VERSION_ARB
			GLMinorVersion,
			0x2094,		// WGL_CONTEXT_FLAGS_ARB
#ifdef _DEBUG
			//0x0001,	// WGL_CONTEXT_DEBUG_BIT
#endif
			0x0002,		// WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
			0x9126,		// WGL_CONTEXT_PROFILE_MASK_ARB
			0x00000001,	// WGL_CONTEXT_CORE_PROFILE_BIT_ARB
			0
		};

		context.hrc = wglCreateContextAttribsARB(hdc, NULL, contextattribs);
		V_RETURN(-1, "PrivateInterface::CreateContext(): Context creation failed", context.hrc);

		std::cout << "Created core profile context (" << contexts.size() << ")...\n";

		success = wglMakeCurrent(hdc, context.hrc);
		V_RETURN(-1, "PrivateInterface::CreateContext(): Could not acquire context", success);

		std::cout << std::endl;
	}

	Quadron::qGLExtensions::QueryFeatures();

	context.hdc = hdc;
	contexts.push_back(context);

	return (int)contexts.size() - 1;
}

void RenderingCore::PrivateInterface::DeleteContext(int id)
{
	OpenGLContext& context = contexts[id];

	if( context.hrc )
	{
		bool isthisactive = (context.hrc == activecontext.hrc);

		if( !wglMakeCurrent(context.hdc, NULL) )
			MYERROR("Could not release context");

		if( !wglDeleteContext(context.hrc) )
			MYERROR("Could not delete context");

		std::cout << "Deleted context (" << id << ")...\n";

		context.hdc = 0;
		context.hrc = 0;

		if( isthisactive )
			activecontext = context;
		else if( activecontext.hrc )
		{
			if( !wglMakeCurrent(activecontext.hdc, activecontext.hrc) )
				MYERROR("Could not reactivate context");
		}
	}
}

bool RenderingCore::PrivateInterface::ActivateContext(int id)
{
	if( id < 0 || id >= (int)contexts.size() )
		return false;

	OpenGLContext& context = contexts[id];

	if( activecontext.hrc != context.hrc )
	{
		if( context.hrc )
		{
			wglMakeCurrent(context.hdc, context.hrc);
			activecontext = context;
		}
	}

	return true;
}

HDC RenderingCore::PrivateInterface::GetDC(int id) const
{
	if( id < 0 || id >= (int)contexts.size() )
		return 0;

	const OpenGLContext& context = contexts[id];
	return context.hdc;
}

void RenderingCore::PrivateInterface::Blit(OpenGLFramebuffer* from, OpenGLFramebuffer* to, GLbitfield flags)
{
	if( !from || !to || from == to || flags == 0 )
		return;

	GLenum filter = GL_LINEAR;

	if( flags & GL_DEPTH_BUFFER_BIT )
		filter = GL_NEAREST;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, from->GetFramebuffer());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to->GetFramebuffer());

	glBlitFramebuffer(
		0, 0, from->GetWidth(), from->GetHeight(),
		0, 0, to->GetWidth(), to->GetHeight(),
		flags, filter);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderingCore::PrivateInterface::Clear(const OpenGLColor& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
	glClearDepth(1.0f);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void RenderingCore::PrivateInterface::Present(int id)
{
	if( id < 0 || id >= (int)contexts.size() )
		return;

	const OpenGLContext& context = contexts[id];

	if( context.hdc )
		SwapBuffers(context.hdc);
}

OpenGLFramebuffer* RenderingCore::PrivateInterface::CreateFramebuffer(GLuint width, GLuint height)
{
	return new OpenGLFramebuffer(width, height);
}

OpenGLScreenQuad* RenderingCore::PrivateInterface::CreateScreenQuad()
{
	return new OpenGLScreenQuad();
}

OpenGLEffect* RenderingCore::PrivateInterface::CreateEffect(const char* vsfile, const char* gsfile, const char* fsfile)
{
	OpenGLEffect* effect = 0;

	if( !GLCreateEffectFromFile(vsfile, gsfile, fsfile, &effect) )
		effect = 0;

	return effect;
}

OpenGLMesh* RenderingCore::PrivateInterface::CreateMesh(const char* file)
{
	OpenGLMesh* mesh = 0;

	if( !GLCreateMeshFromQM(file, 0, 0, &mesh) )
		mesh = 0;

	return mesh;
}

OpenGLMesh* RenderingCore::PrivateInterface::CreateMesh(GLuint numvertices, GLuint numindices, GLuint flags, OpenGLVertexElement* decl)
{
	OpenGLMesh* mesh = 0;

	if( !GLCreateMesh(numvertices, numindices, flags, decl, &mesh) )
		mesh = 0;

	return mesh;
}

// *****************************************************************************************************************************
//
// Core internal tasks
//
// *****************************************************************************************************************************

class UniverseCreatorTask : public RenderingCore::IRenderingTask
{
	enum ContextAction
	{
		Create = 0,
		Delete = 1
	};

private:
	RenderingCore::PrivateInterface* privinterf;
	HDC hdc;
	int contextid;
	int contextaction;

public:
	UniverseCreatorTask(RenderingCore::PrivateInterface* interf, HDC dc)
		: RenderingCore::IRenderingTask(-1)
	{
		privinterf		= interf;
		hdc				= dc;
		contextid		= -1;
		contextaction	= Create;
	}

	UniverseCreatorTask(RenderingCore::PrivateInterface* interf, int id)
		: RenderingCore::IRenderingTask(0)
	{
		privinterf		= interf;
		hdc				= 0;
		contextid		= id;
		contextaction	= Delete;
	}

	void Dispose()
	{
	}

	void Execute(IRenderingContext* context)
	{
		if( contextaction == Create )
			contextid = privinterf->CreateContext(hdc);
		else if( contextaction == Delete )
			privinterf->DeleteContext(contextid);
	}

	inline int GetContextID() const {
		return contextid;
	}
};

// *****************************************************************************************************************************
//
// RenderingCore::IRenderingTask impl
//
// *****************************************************************************************************************************

RenderingCore::IRenderingTask::IRenderingTask(int universe)
{
	universeid = universe;
	disposing = false;
}

RenderingCore::IRenderingTask::~IRenderingTask()
{
}

void RenderingCore::IRenderingTask::Release()
{
	// NOTE: runs on any other thread
	Wait();

	disposing = true;
	GetRenderingCore()->AddTask(this);
}

// *****************************************************************************************************************************
//
// RenderingCore impl
//
// *****************************************************************************************************************************

RenderingCore::RenderingCore()
{
	if( !wglCreateContextAttribsARB )
		SetupCoreProfile();

	privinterf = new PrivateInterface();

	thread.Attach(this, &RenderingCore::THREAD_Run);
	thread.Start();
}

RenderingCore::~RenderingCore()
{
	thread.Stop();
	delete privinterf;
}

int RenderingCore::CreateUniverse(HDC hdc)
{
	UniverseCreatorTask* creator = new UniverseCreatorTask(privinterf, hdc);

	tasks.push(creator);
	creator->Wait();

	int id = creator->GetContextID();
	delete creator;

	return id;
}

void RenderingCore::DeleteUniverse(int id)
{
	UniverseCreatorTask* deleter = new UniverseCreatorTask(privinterf, id);

	tasks.push(deleter);
	deleter->Wait();

	delete deleter;
}

void RenderingCore::AddTask(IRenderingTask* task)
{
	task->finished.Halt();
	tasks.push(task);
}

void RenderingCore::Shutdown()
{
	thread.Stop();
	thread.Close();

	delete _inst;
	_inst = 0;
}

bool RenderingCore::SetupCoreProfile()
{
	HWND dummy = CreateWindow("TestClass", "Dummy", WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
		0, 0, 100, 100, 0, 0, GetModuleHandle(0), 0);

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32, // color
		0, 0, 0, 0, 0, 0,
		0, // alpha
		0, 0, 0, 0, 0, 0,
		24, 8, 0, // depth, stencil, aux
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	HDC hdc = GetDC(dummy);
	V_RETURN(false, "InitGL(): Could not get device context", hdc);
	
	int pixelformat = ChoosePixelFormat(hdc, &pfd);
	V_RETURN(false, "InitGL(): Could not find suitable pixel format", pixelformat);

	BOOL success = SetPixelFormat(hdc, pixelformat, &pfd);
	V_RETURN(false, "InitGL(): Could not setup pixel format", success);

	HGLRC hrc = wglCreateContext(hdc);
	V_RETURN(false, "InitGL(): Context creation failed", hrc);

	success = wglMakeCurrent(hdc, hrc);
	V_RETURN(false, "InitGL(): Could not acquire context", success);

	const char* str = (const char*)glGetString(GL_VENDOR);
	std::cout << "Vendor: " << str << "\n";

	str = (const char*)glGetString(GL_RENDERER);
	std::cout << "Renderer: " << str << "\n";

	str = (const char*)glGetString(GL_VERSION);
	std::cout << "OpenGL version: " << str << "\n\n";

	sscanf_s(str, "%1d.%2d %*s", &GLMajorVersion, &GLMinorVersion);

	if( GLMajorVersion < 3 || (GLMajorVersion == 3 && GLMinorVersion < 2) )
	{
		std::cout << "Device does not support OpenGL 3.2\n";
		return false;
	}

	if( Quadron::wIsSupported("WGL_ARB_pixel_format", hdc) )
	{
		std::cout << "WGL_ARB_pixel_format present, querying pixel formats...\n";

		wglGetPixelFormatAttribivARB = (WGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
		wglGetPixelFormatAttribfvARB = (WGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribfvARB");
		wglChoosePixelFormatARB = (WGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
		
		if( Quadron::wIsSupported("WGL_ARB_create_context", hdc) &&
			Quadron::wIsSupported("WGL_ARB_create_context_profile", hdc) )
		{
			wglCreateContextAttribsARB = (WGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
		}
	}

	success = wglMakeCurrent(hdc, NULL);
	V_RETURN(false, "InitGL(): Could not release dummy context", success);

	success = wglDeleteContext(hrc);
	V_RETURN(false, "InitGL(): Could not delete dummy context", success);

	ReleaseDC(dummy, hdc);
	DestroyWindow(dummy);

	return true;
}

void RenderingCore::THREAD_Run()
{
	CoInitializeEx(0, COINIT_MULTITHREADED);

	while( true )
	{
		IRenderingTask* action = tasks.pop();

		if( action->GetUniverseID() == -1 )
		{
			// this is a context creator action
			// NOTE: unsafe...
			action->Execute(privinterf);
		}
		else if( privinterf->ActivateContext(action->GetUniverseID()) )
		{
			if( action->disposing )
			{
				action->Dispose();

				delete action;
				action = 0;
			}
			else
				action->Execute(privinterf);
		}

		if( action )
			action->finished.Fire();
	}
}
