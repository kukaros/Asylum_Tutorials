
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "GdiPlus.lib")

#include <iostream>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "win32window.h"
#include "renderingcore.h"

extern void MainWindow_Created(Win32Window*);
extern void MainWindow_Closing(Win32Window*);
extern void MainWindow_KeyPress(Win32Window*, WPARAM);
extern void MainWindow_Render(Win32Window*, float, float);

int main(int argc, char* argv[])
{
	RECT	workarea;
	LONG	wawidth;
	LONG	waheight;

	CoInitializeEx(0, COINIT_MULTITHREADED);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea, 0);

	workarea.left += 5;
	workarea.top += 5;
	workarea.right -= 10;
	workarea.bottom -= 10;

	wawidth = workarea.right - workarea.left;
	waheight = workarea.bottom - workarea.top;

	{
		Win32Window mainwindow(workarea.left, workarea.top, wawidth / 2, waheight / 2);

		mainwindow.CreateCallback	= &MainWindow_Created;
		mainwindow.CloseCallback	= &MainWindow_Closing;
		mainwindow.KeyPressCallback	= &MainWindow_KeyPress;
		mainwindow.RenderCallback	= &MainWindow_Render;

		mainwindow.SetTitle("3D window");
		mainwindow.MessageHook();
	}

	GetRenderingCore()->Shutdown();

	_CrtDumpMemoryLeaks();

#ifdef _DEBUG
	system("pause");
#endif

	return 0;
}
