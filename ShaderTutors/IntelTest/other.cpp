//*************************************************************************************************************
#define TITLE				"Intel FBO blit test"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

#include <windows.h>
#include <GL/gl.h>
#include <iostream>
#include <cmath>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

HWND	hwnd	= NULL;
HDC		hdc		= NULL;
HGLRC	hrc		= NULL;

RECT	workarea;
DEVMODE	devmode;
long	screenwidth		= 800;
long	screenheight	= 600;

bool InitScene();
void UninitScene();
void Update(float delta);
void Render(float alpha, float elapsedtime);
void KeyPress(WPARAM wparam);

bool IsSupported(const char* (APIENTRY *func)(HDC), HDC hdc, const char* name)
{
	const char *ext = 0, *start;
	const char *loc, *term;

	loc = strchr(name, ' ');

	if( loc || *name == '\0' )
		return false;

	ext = (const char*)func(hdc);
	start = ext;

	for( ;; )
	{
		if( !(loc = strstr(start, name)) )
			break;

		term = loc + strlen(name);

		if( loc == start || *(loc - 1) == ' ' )
		{
			if( *term == ' ' || *term == '\0' )
				return true;
		}

		start = term;
	}

	return false;
}

void DrawPlanes()
{
	glBegin(GL_QUADS);
	{
		glColor3ub(255, 0, 0);
		glVertex3f(-3, -0.5, -2);
		glVertex3f(-1, -0.5, -2);
		glVertex3f(-1, -0.5,  2);
		glVertex3f(-3, -0.5,  2);
	}
	glEnd();

	glBegin(GL_QUADS);
	{
		glColor3ub(0, 255, 0);
		glVertex3f(-1, 0, -2);
		glVertex3f( 1, 0, -2);
		glVertex3f( 1, 0,  2);
		glVertex3f(-1, 0,  2);
	}
	glEnd();

	glBegin(GL_QUADS);
	{
		glColor3ub(0, 0, 255);
		glVertex3f(1, 0.5, -2);
		glVertex3f(3, 0.5, -2);
		glVertex3f(3, 0.5,  2);
		glVertex3f(1, 0.5,  2);
	}
	glEnd();
}
//*************************************************************************************************************
void DrawCube()
{
	glBegin(GL_QUADS);
	{
		glNormal3f(0.0f, 0.0f, 1.0f);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glVertex3f( 1.0f, -1.0f, 1.0f);

		glNormal3f(0.0f, 0.0f, -1.0f);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);

		glNormal3f(0.0f, 1.0f, 0.0f);
		glVertex3f( 1.0f, 1.0f, -1.0f);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glVertex3f( 1.0f, 1.0f, 1.0f);

		glNormal3f(0.0f,-1.0f, 0.0f);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glVertex3f( 1.0f, -1.0f, -1.0f);

		glNormal3f(1.0f, 0.0f, 0.0f);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glVertex3f(1.0f, -1.0f, 1.0f);

		glNormal3f(-1.0f, 0.0f, 0.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glVertex3f(-1.0f,  1.0f, 1.0f);
		glVertex3f(-1.0f,  1.0f, -1.0f);
	}
	glEnd();
}
//*************************************************************************************************************
bool InitGL(HWND hwnd)
{
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

	hdc = GetDC(hwnd);
	V_RETURN(false, "InitGL(): Could not get device context", hdc);
	
	int pixelformat = ChoosePixelFormat(hdc, &pfd);
	V_RETURN(false, "InitGL(): Could not find suitable pixel format", pixelformat);

	BOOL success = SetPixelFormat(hdc, pixelformat, &pfd);
	V_RETURN(false, "InitGL(): Could not setup pixel format", success);

	hrc = wglCreateContext(hdc);
	V_RETURN(false, "InitGL(): Context creation failed", hrc);

	success = wglMakeCurrent(hdc, hrc);
	V_RETURN(false, "InitGL(): Could not acquire context", success);

	return true;
}
//*************************************************************************************************************
void Adjust(tagRECT& out, long& width, long& height, DWORD style, DWORD exstyle, bool menu = false)
{
	long w = workarea.right - workarea.left;
	long h = workarea.bottom - workarea.top;

	out.left = (w - width) / 2;
	out.top = (h - height) / 2;
	out.right = (w + width) / 2;
	out.bottom = (h + height) / 2;

	AdjustWindowRectEx(&out, style, menu, 0);

	long windowwidth = out.right - out.left;
	long windowheight = out.bottom - out.top;

	long dw = windowwidth - width;
	long dh = windowheight - height;

	if( windowheight > h )
	{
		float ratio = (float)width / (float)height;
		float realw = (float)(h - dh) * ratio + 0.5f;

		windowheight = h;
		windowwidth = (long)floor(realw) + dw;
	}

	if( windowwidth > w )
	{
		float ratio = (float)height / (float)width;
		float realh = (float)(w - dw) * ratio + 0.5f;

		windowwidth = w;
		windowheight = (long)floor(realh) + dh;
	}

	out.left = workarea.left + (w - windowwidth) / 2;
	out.top = workarea.top + (h - windowheight) / 2;
	out.right = workarea.left + (w + windowwidth) / 2;
	out.bottom = workarea.top + (h + windowheight) / 2;

	width = windowwidth - dw;
	height = windowheight - dh;
}
//*************************************************************************************************************
LRESULT WINAPI WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	if( hWnd != hwnd )
		return DefWindowProc(hWnd, msg, wParam, lParam);

	switch( msg )
	{
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);

		// release context
		if( hrc )
		{
			UninitScene();

			if( !wglMakeCurrent(hdc, NULL) )
				MYERROR("Could not release context");

			if( !wglDeleteContext(hrc) )
				MYERROR("Could not delete context");

			hrc = NULL;
		}

		if( hdc && !ReleaseDC(hwnd, hdc) )
			MYERROR("Could not release device context");

		hdc = NULL;
		DestroyWindow(hWnd);

		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	
	case WM_KEYUP:
		switch( wParam )
		{
		case VK_ESCAPE:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		default:
			KeyPress(wParam);
			break;
		}

		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}
//*************************************************************************************************************
int main(int argc, char* argv[])
{
	LARGE_INTEGER qwTicksPerSec = { 0, 0 };
	LARGE_INTEGER qwTime;
	LONGLONG tickspersec;
	double last, current;
	double delta, accum = 0;

	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		(WNDPROC)WndProc,
		0L,
		0L,
		GetModuleHandle(NULL),
		NULL, NULL, NULL, NULL, "TestClass", NULL
	};

	RegisterClassEx(&wc);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea, 0);
	
	RECT rect = { 0, 0, screenwidth, screenheight };
	DWORD style = WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	memset(&devmode, 0, sizeof(DEVMODE));

	devmode.dmSize			= sizeof(DEVMODE);
	devmode.dmBitsPerPel	= 32;
	devmode.dmPelsWidth		= screenwidth;
	devmode.dmPelsHeight	= screenheight;
	devmode.dmFields		= DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

	// ablakos mód
	style |= WS_SYSMENU|WS_BORDER|WS_CAPTION|WS_MINIMIZEBOX;
	Adjust(rect, screenwidth, screenheight, style, true);
	
	hwnd = CreateWindowA("TestClass", TITLE, style,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, wc.hInstance, NULL);
	
	if( !hwnd )
	{
		MYERROR("Could not create window");
		goto _end;
	}

	if( !InitGL(hwnd) )
	{
		MYERROR("Failed to initialize OpenGL");
		goto _end;
	}
	
	if( !InitScene() )
	{
		MYERROR("Failed to initialize scene");
		goto _end;
	}
	
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);
	
	// timer
	QueryPerformanceFrequency(&qwTicksPerSec);
	tickspersec = qwTicksPerSec.QuadPart;

	QueryPerformanceCounter(&qwTime);
	last = (double)qwTime.QuadPart / (double)tickspersec;

	while( msg.message != WM_QUIT )
	{
		QueryPerformanceCounter(&qwTime);

		current = (double)qwTime.QuadPart / (double)tickspersec;
		delta = (current - last);

		last = current;
		accum += delta;

		while( accum > 0.0333f )
		{
			accum -= 0.0333f;

			while( PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) )
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			Update(0.0333f);
		}

		if( msg.message != WM_QUIT )
			Render((float)accum / 0.0333f, (float)delta);
	}

_end:
	std::cout << "Exiting...\n";

	UnregisterClass("TestClass", wc.hInstance);
	_CrtDumpMemoryLeaks();

	return 0;
}
//*************************************************************************************************************
