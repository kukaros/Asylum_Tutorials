//*************************************************************************************************************
#define TITLE				"WGL test"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

#include <windows.h>
#include <GL/gl.h>
#include <iostream>
#include <cmath>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#define IDM_OPEN_ITEM			1001
#define IDM_EXIT_ITEM			1004
#define IDM_ABOUT_ITEM			1005
#define IDM_FULLSCREEN_ITEM		1006

#define WM_DEACTIVATE_FILE		WM_USER + 1
#define WM_ACTIVATE_FILE		WM_USER + 2
#define WM_DISABLE_ACTIVE		WM_USER + 3
#define WM_ENABLE_ACTIVE		WM_USER + 4

HWND	hwnd	= NULL;
HDC		hdc		= NULL;
HGLRC	hrc		= NULL;

RECT	workarea;
DEVMODE	devmode;
long	screenwidth		= 800;
long	screenheight	= 600;
bool	fullscreen		= false;
bool	active			= false;
bool	minimized		= false;

bool InitScene();
void LoadTexture(const std::wstring& file);
void UninitScene();
void Update(float delta);
void Render(float alpha, float elapsedtime);

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

bool InitGL(HWND hwnd)
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

	hdc = GetDC(dummy);
	V_RETURN(false, "InitGL(): Could not get device context", hdc);
	
	int pixelformat = ChoosePixelFormat(hdc, &pfd);
	V_RETURN(false, "InitGL(): Could not find suitable pixel format", pixelformat);

	BOOL success = SetPixelFormat(hdc, pixelformat, &pfd);
	V_RETURN(false, "InitGL(): Could not setup pixel format", success);

	hrc = wglCreateContext(hdc);
	V_RETURN(false, "InitGL(): Context creation failed", hrc);

	success = wglMakeCurrent(hdc, hrc);
	V_RETURN(false, "InitGL(): Could not acquire context", success);

	std::cout << "\nWGL test program by Asylum\n\n";

	const char* str = (const char*)glGetString(GL_VENDOR);
	std::cout << "Vendor: " << str << "\n";

	str = (const char*)glGetString(GL_RENDERER);
	std::cout << "Renderer: " << str << "\n";

	str = (const char*)glGetString(GL_VERSION);
	std::cout << "OpenGL version: " << str << "\n\n";

	typedef const char* (APIENTRY *WGLGETEXTENSIONSSTRINGARBPROC)(HDC hdc);
	WGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (WGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");

	if( wglGetExtensionsStringARB )
	{
		if( IsSupported(wglGetExtensionsStringARB, hdc, "WGL_ARB_pixel_format") )
		{
			std::cout << "WGL_ARB_pixel_format present, querying pixel formats...\n";

			typedef BOOL (APIENTRY *WGLGETPIXELFORMATATTRIBIVARBPROC)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
			typedef BOOL (APIENTRY *WGLGETPIXELFORMATATTRIBFVARBPROC)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
			typedef BOOL (APIENTRY *WGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

			WGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB = (WGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
			WGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB = (WGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribfvARB");
			WGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (WGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

			int attrib[128];
			int values[128];
			int i;
			int count;
			int good = 0;
			UINT numformats;

			memset(attrib, 0, sizeof(attrib));
			memset(values, 0, sizeof(values));

			attrib[0] = 0x2000; // WGL_NUMBER_PIXEL_FORMATS_ARB;
			wglGetPixelFormatAttribivARB(hdc, 0, 0, 1, attrib, values);

			count = values[0];
			std::cout << "Number of pixel formats: " << count << "\n\n";

			for( int j = 1; j <= count; ++j )
			{
				i = 0;

				attrib[i++] = 0x2001; // WGL_DRAW_TO_WINDOW_ARB;
				attrib[i++] = 0x2003; // WGL_ACCELERATION_ARB;
				attrib[i++] = 0x2007; // WGL_SWAP_METHOD_ARB
				attrib[i++] = 0x2010; // WGL_SUPPORT_OPENGL_ARB
				attrib[i++] = 0x2011; // WGL_DOUBLE_BUFFER_ARB
				attrib[i++] = 0x2012; // WGL_STEREO_ARB
				attrib[i++] = 0x2013; // WGL_PIXEL_TYPE_ARB
				attrib[i++] = 0x2014; // WGL_COLOR_BITS_ARB
				attrib[i++] = 0x201B; // WGL_ALPHA_BITS_ARB
				attrib[i++] = 0x2022; // WGL_DEPTH_BITS_ARB
				attrib[i++] = 0x2023; // WGL_STENCIL_BITS_ARB
				attrib[i++] = 0;

				success = wglGetPixelFormatAttribivARB(hdc, j, 0, (i - 1), attrib, values);

				if( !success )
				{
					std::cout << "Failed to query pixel format " << j << "\n";
					continue;
				}

				if( values[1] == 0x2027 && // WGL_FULL_ACCELERATION_ARB
					values[6] == 0x202B && // WGL_TYPE_RGBA_ARB
					values[7] > 16 &&
					attrib[0] != 0 &&
					values[3] != 0 &&
					values[4] != 0 )
				{
					if( values[5] == 1 )
						std::cout << "Pixel format " << j << " has stereo: ";
					else
						std::cout << "Pixel format " << j << " is good:    ";

					std::cout << values[7] << ", " << values[8] << ", " << values[9] << ", " << values[10];

					if( values[2] == 0x2029 ) // WGL_SWAP_COPY_ARB
						std::cout << ", WGL_SWAP_COPY";

					std::cout << std::endl;
					++good;
				}
			}

			std::cout << "\nFound " << good <<" suitable pixel formats\n";

			i = 0;
			
			attrib[i++] = 0x2001;		// WGL_DRAW_TO_WINDOW_ARB;
			attrib[i++] = TRUE;
			attrib[i++] = 0x2003;		// WGL_ACCELERATION_ARB;
			attrib[i++] = 0x2027;		// WGL_FULL_ACCELERATION_ARB
			attrib[i++] = 0x2010;		// WGL_SUPPORT_OPENGL_ARB
			attrib[i++] = TRUE;
			attrib[i++] = 0x2011;		// WGL_DOUBLE_BUFFER_ARB
			attrib[i++] = TRUE;
			//attrib[i++] = 0x2007;		// WGL_SWAP_METHOD_ARB
			//attrib[i++] = 0x2029;		// WGL_SWAP_COPY_ARB
			//attrib[i++] = 0x2012;		// WGL_STEREO_ARB
			//attrib[i++] = TRUE;
			attrib[i++] = 0x2013;		// WGL_PIXEL_TYPE_ARB
			attrib[i++] = 0x202B;		// WGL_TYPE_RGBA_ARB
			attrib[i++] = 0x2014;		// WGL_COLOR_BITS_ARB
			attrib[i++] = pfd.cColorBits = 32;
			attrib[i++] = 0x201B;		// WGL_ALPHA_BITS_ARB
			attrib[i++] = pfd.cAlphaBits = 0;
			attrib[i++] = 0x2022;		// WGL_DEPTH_BITS_ARB
			attrib[i++] = pfd.cDepthBits = 24;
			attrib[i++] = 0x2023;		// WGL_STENCIL_BITS_ARB
			attrib[i++] = pfd.cStencilBits = 8;
			attrib[i++] = 0;

			if( attrib[1] )
				pfd.dwFlags |= PFD_DRAW_TO_WINDOW;

			if( attrib[5] )
				pfd.dwFlags |= PFD_SUPPORT_OPENGL;

			if( attrib[7] )
				pfd.dwFlags |= PFD_DOUBLEBUFFER;

			if( attrib[9] == 0x2029 )
				pfd.dwFlags |= PFD_SWAP_COPY;

			wglChoosePixelFormatARB(hdc, attrib, 0, 1, &pixelformat, &numformats);

			if( numformats != 0 )
			{
				std::cout << "Selected pixel format: " << pixelformat <<"\n";

				success = wglMakeCurrent(hdc, NULL);
				V_RETURN(false, "InitGL(): Could not release dummy context", success);

				success = wglDeleteContext(hrc);
				V_RETURN(false, "InitGL(): Could not delete dummy context", success);

				DestroyWindow(dummy);
				dummy = 0;
				hdc = GetDC(hwnd);

				int success = SetPixelFormat(hdc, pixelformat, &pfd);
				V_RETURN(false, "InitGL(): Could not setup pixel format", success);

				hrc = wglCreateContext(hdc);
				V_RETURN(false, "InitGL(): Context creation failed", hrc);

				success = wglMakeCurrent(hdc, hrc);
				V_RETURN(false, "InitGL(): Could not acquire context", success);

				std::cout << std::endl;
			}
			else
				std::cout << "wglChoosePixelFormat failed, using classic context...\n";
		}
		else
			std::cout << "WGL_ARB_pixel_format not supported\n";
	}
	else
		std::cout << "wglGetExtensionsStringARB not supported\n";

	if( dummy )
	{
		std::cout << "Selected pixel format: " << pixelformat <<"\n";

		DestroyWindow(dummy);
		hdc = GetDC(hwnd);

		int success = SetPixelFormat(hdc, pixelformat, &pfd);
		V_RETURN(false, "InitGL(): Could not setup pixel format", success);

		hrc = wglCreateContext(hdc);
		V_RETURN(false, "InitGL(): Context creation failed", hrc);

		success = wglMakeCurrent(hdc, hrc);
		V_RETURN(false, "InitGL(): Could not acquire context", success);

		std::cout << std::endl;
	}

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
void ToggleFullscreen()
{
	RECT rect;
	DWORD style = WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	if( fullscreen )
	{
		style |= WS_SYSMENU|WS_BORDER|WS_CAPTION|WS_MINIMIZEBOX;

		// fullscreen -> windowed
		ChangeDisplaySettings(NULL, 0);
		Adjust(rect, screenwidth, screenheight, style, true);

		SetWindowLong(hwnd, GWL_STYLE, style);
		SetWindowPos(hwnd, HWND_NOTOPMOST, rect.left, rect.top, rect.right - rect.left,
			rect.bottom - rect.top, SWP_FRAMECHANGED);

		fullscreen = false;
		std::cout << "Application changed to windowed mode.\n";
	}
	else
	{
		style |= WS_POPUP;

		// windowed -> fullscreen
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, screenwidth, screenheight, SWP_NOACTIVATE);
		SetWindowLong(hwnd, GWL_STYLE, style);
		SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);

		SetForegroundWindow(hwnd);

		if( DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettings(&devmode, CDS_FULLSCREEN) )
			MYERROR("Could not change display settings")
		else
		{
			fullscreen = true;
			std::cout << "Application changed to fullscreen mode.\n";
		}
	}
}
//*************************************************************************************************************
void HideFullscreenWindow()
{
	if( fullscreen )
	{
		ShowWindow(hwnd, SW_MINIMIZE);
		ChangeDisplaySettings(NULL, 0);
	}
}
//*************************************************************************************************************
void ShowFullscreenWindow()
{
	if( fullscreen )
	{
		SetForegroundWindow(hwnd);
		ShowWindow(hwnd, SW_RESTORE);

		if( DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettings(&devmode, CDS_FULLSCREEN) )
			MYERROR("Could not change display settings")
	}
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

		if( fullscreen )
		{
			ChangeDisplaySettings(NULL, 0);
			fullscreen = false;
		}

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

	case WM_COMMAND:
		switch( LOWORD(wParam) )
		{
		case IDM_OPEN_ITEM: {
			OPENFILENAMEW ofn;
			std::wstring wname;
			BOOL result;

			memset(&ofn, 0, sizeof(OPENFILENAMEW));
			wname.resize(2048);

			ofn.lStructSize				= sizeof(OPENFILENAMEW);
			ofn.lpstrCustomFilter		= 0;
			ofn.nMaxCustFilter			= 0;
			ofn.nFilterIndex			= 0;
			ofn.nMaxFile				= 2048;
			ofn.lpstrFileTitle			= 0;
			ofn.nMaxFileTitle			= 0;
			ofn.Flags					= OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
			ofn.nFileOffset				= 0;
			ofn.nFileExtension			= 0;
			ofn.lpstrDefExt				= 0;
			ofn.lCustData				= 0;
			ofn.lpfnHook				= 0;
			ofn.lpTemplateName			= 0;

			ofn.hwndOwner				= hwnd;
			ofn.lpstrFilter				= L"Image files (*.jpg;*.bmp;*.png)\0*.jpg;*.bmp;*.png\0\0";
			ofn.lpstrFile				= &wname[0];
			ofn.lpstrInitialDir			= 0;

			SendMessage(hwnd, WM_DISABLE_ACTIVE, 0, 0);

			if( fullscreen )
			{
				ToggleFullscreen();
				Render(0, 1);

				result = GetOpenFileNameW(&ofn);
				ToggleFullscreen();
			}
			else
				result = GetOpenFileNameW(&ofn);

			SendMessage(hwnd, WM_ENABLE_ACTIVE, 0, 0);

			if( result > 0 )
				LoadTexture(wname);

			}
			return 0; // do not send activate messages

		case IDM_EXIT_ITEM:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		case IDM_ABOUT_ITEM:
			SendMessage(hwnd, WM_DISABLE_ACTIVE, 0, 0);

			MessageBoxA(hwnd,
				"This test program helps to identify problems regarding the WGL_pixel_format_ARB extension. "
				"You should see a gray cube rotating before a blue background.\n\n"
				"You can test the following things:\n"

				"1) Toggle between fullscreen and windowed mode\n"
				"2) When in fullscreen mode, check if the About item is working. "
				"If it pops up in the background, press space.\n"
				"3) Also in fullscreen mode, test Alt-Tab\n"
				"4) Use the Open item to give the cube a texture\n\n"

				"Note that when in fullscreen mode, the application has exclusive focus, and if you click another window,"
				"then the program acts as if you pressed Alt-Tab.\n\n"

				"If you find any problems, right click on the console window's title bar, "
				"and press Edit -> Select All, then right click on the selection, and paste it into a text document.",
				"About", MB_OK);

			SendMessage(hwnd, WM_ENABLE_ACTIVE, 0, 0);
			return 0; // do not send activate messages

		case IDM_FULLSCREEN_ITEM:
			ToggleFullscreen();
			return 0; // do not send activate messages
		}

	case WM_DEACTIVATE_FILE:
		SendMessage(hWnd, WM_ACTIVATE, WA_INACTIVE, 0);
		minimized = true;
		return 0;

	case WM_ACTIVATE_FILE:
		minimized = false;
		SendMessage(hWnd, WM_ACTIVATE, WA_ACTIVE, 0);
		return 0;

	case WM_DISABLE_ACTIVE:
		active = false;
		minimized = true;
		break;

	case WM_ENABLE_ACTIVE:
		active = true;
		minimized = false;
		break;

	case WM_ACTIVATE:
	case WM_ACTIVATEAPP:
		switch( wParam )
		{
		case WA_INACTIVE:
			if( active )
			{
				active = false;
				HideFullscreenWindow();

				std::cout << "Application deactivated.\n";
			}

			break;

		case WA_ACTIVE:
		case WA_CLICKACTIVE:
			if( !active && !minimized )
			{
				ShowFullscreenWindow();
				active = true;

				std::cout << "Application activated.\n";
			}

			break;

		default:
			break;
		}

		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	
	case WM_KEYUP:
		switch(wParam)
		{
		case VK_ESCAPE:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
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

	// ablak osztály
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

	HMENU menu = CreateMenu();
	HMENU submenu1 = CreatePopupMenu();
	HMENU submenu2 = CreatePopupMenu();
	HMENU submenu3 = CreatePopupMenu();
	
	AppendMenuA(submenu1, MF_STRING, IDM_OPEN_ITEM, "&Open");
	AppendMenuA(submenu1, MF_SEPARATOR, 0, 0);
	AppendMenuA(submenu1, MF_STRING, IDM_EXIT_ITEM, "&Exit");
	AppendMenuA(submenu2, MF_STRING, IDM_FULLSCREEN_ITEM, "&Toggle fullscreen");
	AppendMenuA(submenu3, MF_STRING, IDM_ABOUT_ITEM, "&About");

	AppendMenuA(menu, MF_STRING|MF_POPUP, (UINT)submenu1, "&File");
	AppendMenuA(menu, MF_STRING|MF_POPUP, (UINT)submenu2, "&Window");
	AppendMenuA(menu, MF_STRING|MF_POPUP, (UINT)submenu3, "&Help");
	
	SetMenu(hwnd, menu);

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
