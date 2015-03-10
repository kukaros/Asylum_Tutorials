
#ifndef _WIN32WINDOW_H_
#define _WIN32WINDOW_H_

#include <Windows.h>
#include <map>

#include "../common/thread.h"

class DrawingItem;

class Win32Window
{
	static LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);

	typedef std::map<HWND, Win32Window*> windowmap;

	static WNDCLASSEX	wc;
	static windowmap	handles;
	static Guard		handlesguard;

private:
	HWND			hwnd;
	HDC				hdc;
	DrawingItem*	drawingitem;
	int				glcontextid;
	long			clientwidth, clientheight;
	short			mousex, mousey;
	short			mousedx, mousedy;
	short			mousedown;

	void UninitOpenGL();

public:
	void (*KeyPressCallback)(Win32Window*, WPARAM);
	void (*MouseMoveCallback)(Win32Window*);
	void (*UpdateCallback)(float);
	void (*RenderCallback)(Win32Window*, float, float);
	void (*CloseCallback)(Win32Window*);
	void (*CreateCallback)(Win32Window*);

	Win32Window(long x, long y, long width, long height);
	~Win32Window();

	void Close();
	void MessageHook();
	void SetTitle(const char* title);
	void SetFocus();

	inline DrawingItem* GetDrawingItem() {
		return drawingitem;
	}

	inline long GetClientWidth() const {
		return clientwidth;
	}

	inline long GetClientHeight() const {
		return clientheight;
	}
};

#endif
