
#pragma comment(lib, "comctl32.lib")

#include <Windows.h>

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		DestroyWindow(hWnd);
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

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	HWND hwnd = 0;

	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		(WNDPROC)WndProc,
		0L, 0L,
		hInst,
		NULL, NULL, NULL, NULL,
		"TestClass", NULL
	};

	RegisterClassEx(&wc);
	
	int w = GetSystemMetrics(0);
	int h = GetSystemMetrics(1);

	DWORD style = WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_SYSMENU|WS_BORDER|WS_CAPTION;

	hwnd = CreateWindowA("TestClass", "Winapi1", style,
		(w - 800) / 2, (h - 600) / 2, 800, 600,
		NULL, NULL, wc.hInstance, NULL);
	
	if( !hwnd )
	{
		MessageBoxA(NULL, "Could not create window!", "Error!", MB_OK);
		goto _end;
	}

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while( msg.message != WM_QUIT )
	{
		while( PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

_end:
	UnregisterClass("TestClass", wc.hInstance);
	return 0;
}

