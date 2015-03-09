
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dwmapi.lib")
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

extern void Window1_Created(Win32Window*);
extern void Window1_Closing(Win32Window*);
extern void Window1_Render(Win32Window*, float, float);

RECT			workarea;
LONG			wawidth;
LONG			waheight;
Win32Window*	window1;

void THREAD1_Run()
{
	CoInitializeEx(0, COINIT_MULTITHREADED);

	window1 = new Win32Window(
		workarea.left + wawidth / 2, workarea.top,
		wawidth / 2, waheight / 2);

	window1->CreateCallback	= &Window1_Created;
	window1->CloseCallback	= &Window1_Closing;
	window1->RenderCallback	= &Window1_Render;

	window1->SetTitle("2D window 1");
	window1->MessageHook();

	delete window1;
	window1 = 0;

	CoUninitialize();
}

int main(int argc, char* argv[])
{
	CoInitializeEx(0, COINIT_MULTITHREADED);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea, 0);

	wawidth = workarea.right - workarea.left;
	waheight = workarea.bottom - workarea.top;

	// other windows
	Thread worker1;

	worker1.Attach(&THREAD1_Run);
	worker1.Start();

	// main window
	{
		Win32Window mainwindow(
			workarea.left, workarea.top,
			wawidth / 2, waheight / 2);

		mainwindow.CreateCallback	= &MainWindow_Created;
		mainwindow.CloseCallback	= &MainWindow_Closing;
		mainwindow.KeyPressCallback	= &MainWindow_KeyPress;
		mainwindow.RenderCallback	= &MainWindow_Render;

		mainwindow.SetTitle("3D window");
		mainwindow.MessageHook();
	}

	// close other windows
	if( window1 )
		window1->Close();

	worker1.Wait();

	GetRenderingCore()->Shutdown();
	CoUninitialize();

	_CrtDumpMemoryLeaks();

#ifdef _DEBUG
	system("pause");
#endif

	return 0;
}
