
	#include <Windows.h>

	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch( msg )
		{
		case WM_CLOSE:
			ShowWindow(hWnd, SW_HIDE);
			DestroyWindow(hWnd);
			break;

		case WM_PAINT:
			{
				HDC hdc = GetDC(hWnd);
				HDC bdc = CreateCompatibleDC(hdc);
				HBITMAP bitmap = CreateCompatibleBitmap(hdc, 400, 100);

				int height = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);

				HFONT font = CreateFont(
					height, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
					DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					CLEARTYPE_QUALITY, FF_DONTCARE, "Consolas");

				HGDIOBJ oldbm = SelectObject(bdc, bitmap);
				HGDIOBJ oldft = SelectObject(bdc, font);

				RECT rc = { 0, 0, 200, 20 };

				FillRect(bdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

				DrawText(bdc, " 1 2 3 4 5 6 7 8 9", 19, &rc, DT_LEFT|DT_EDITCONTROL);
				BitBlt(hdc, 20, 20, rc.right - rc.left, rc.bottom - rc.top, bdc, 0, 0, SRCCOPY);

				SelectObject(bdc, oldbm);
				SelectObject(bdc, oldft);

				DeleteObject(bitmap);
				DeleteObject(font);
				DeleteDC(bdc);
			}
			return 1;

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

		hwnd = CreateWindowA("TestClass", "Winapi3", style,
			(w - 400) / 2, (h - 200) / 2, 400, 200,
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
