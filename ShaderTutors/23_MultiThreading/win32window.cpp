
#include "win32window.h"
#include "drawingitem.h"
#include "renderingcore.h"

Win32Window::windowmap Win32Window::handles;
WNDCLASSEX Win32Window::wc;

LRESULT WINAPI Win32Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Win32Window::windowmap::iterator it = Win32Window::handles.find(hWnd);

	if( it == Win32Window::handles.end() )
		return DefWindowProc(hWnd, msg, wParam, lParam);

	switch( msg )
	{
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);

		if( it->second->CloseCallback )
			(*it->second->CloseCallback)(it->second);

		it->second->UninitOpenGL();
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	
	case WM_KEYUP:
		if( it->second->KeyPressCallback )
			it->second->KeyPressCallback(it->second, wParam);

		break;

	case WM_MOUSEMOVE: {
		short x = (short)(lParam & 0xffff);
		short y = (short)((lParam >> 16) & 0xffff);

		it->second->mousedx += x - it->second->mousex;
		it->second->mousedy += y - it->second->mousey;

		it->second->mousex = x;
		it->second->mousey = y;

		if( it->second->MouseMoveCallback )
			it->second->MouseMoveCallback(it->second);

		} break;

	case WM_LBUTTONDOWN:
		it->second->mousedown = 1;

		if( it->second->MouseMoveCallback )
			it->second->MouseMoveCallback(it->second);

		break;

	case WM_RBUTTONDOWN:
		it->second->mousedown = 2;

		if( it->second->MouseMoveCallback )
			it->second->MouseMoveCallback(it->second);

		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		it->second->mousedown = 0;

		if( it->second->MouseMoveCallback )
			it->second->MouseMoveCallback(it->second);

		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

Win32Window::Win32Window(long x, long y, long width, long height)
{
	if( handles.size() == 0 )
	{
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.style			= CS_OWNDC;
		wc.lpfnWndProc		= (WNDPROC)WndProc;
		wc.cbClsExtra		= 0L;
		wc.cbWndExtra		= 0L;
		wc.hInstance		= GetModuleHandle(NULL);
		wc.hIcon			= NULL;
		wc.hCursor			= LoadCursor(0, IDC_ARROW);
		wc.hbrBackground	= NULL;
		wc.lpszMenuName		= NULL;
		wc.lpszClassName	= "TestClass";
		wc.hIconSm			= NULL;

		RegisterClassEx(&wc);
	}

	KeyPressCallback	= 0;
	MouseMoveCallback	= 0;
	UpdateCallback		= 0;
	RenderCallback		= 0;
	CloseCallback		= 0;
	CreateCallback		= 0;

	RECT rect = { x, y, width, height };
	DWORD style = WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	style |= WS_SYSMENU|WS_BORDER|WS_CAPTION|WS_MINIMIZEBOX;
	Adjust(rect, x, y, width, height, style, 0, false);

	hwnd = CreateWindowA("TestClass", "Win32Window", style,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, wc.hInstance, NULL);

	hdc = GetDC(hwnd);
	glcontextid	= -1;
	drawingitem	= 0;

	if( hwnd )
	{
		hdc = GetDC(hwnd);
		glcontextid = GetRenderingCore()->CreateUniverse(hdc);

		if( glcontextid != -1 )
		{
			drawingitem = new DrawingItem(
				glcontextid,
				(unsigned int)(rect.right - rect.left),
				(unsigned int)(rect.bottom - rect.top));
		}
	}

	handles.insert(windowmap::value_type(hwnd, this));
}

Win32Window::~Win32Window()
{
	ReleaseDC(hwnd, hdc);
	handles.erase(hwnd);

	hdc = 0;

	if( handles.size() == 0 )
		UnregisterClass("TestClass", wc.hInstance);
}

void Win32Window::Adjust(tagRECT& out, long x, long y, long width, long height, DWORD style, DWORD exstyle, bool menu)
{
	out.left	= x;
	out.top		= y;
	out.right	= x + width;
	out.bottom	= y + height;

	AdjustWindowRectEx(&out, style, menu, exstyle);

	out.right	= x + (out.right - out.left);
	out.bottom	= y + (out.bottom - out.top);
	out.left	= x;
	out.top		= y;
}

void Win32Window::UninitOpenGL()
{
	if( drawingitem )
		delete drawingitem;

	if( glcontextid != -1 )
		GetRenderingCore()->DeleteUniverse(glcontextid);
}

void Win32Window::Present()
{
	SwapBuffers(hdc);
}

void Win32Window::SetTitle(const char* title)
{
	if( hwnd )
		SetWindowText(hwnd, title);
}

void Win32Window::Close()
{
	if( hwnd )
		SendMessage(hwnd, WM_CLOSE, 0, 0);
}

void Win32Window::MessageHook()
{
	LARGE_INTEGER	qwTicksPerSec = { 0, 0 };
	LARGE_INTEGER	qwTime;
	LONGLONG		tickspersec;
	MSG				msg;
	POINT			p;
	double			last, current;
	double			delta, accum = 0;

	if( CreateCallback )
		(*CreateCallback)(this);

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	ZeroMemory(&msg, sizeof(msg));

	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);
	
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

		mousedx = mousedy = 0;

		while( accum > 0.1f )
		{
			accum -= 0.1f;

			while( PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) )
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if( UpdateCallback )
				UpdateCallback(0.1f);
		}

		if( msg.message != WM_QUIT )
		{
			if( RenderCallback ) // && glcontextid != -1 )
				RenderCallback(this, (float)accum / 0.1f, (float)delta);
		}
	}
}
