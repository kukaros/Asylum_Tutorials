//*************************************************************************************************************
#define TITLE				"Tutorial 17 - GL_ARB_texture_env_combine"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

#include <Windows.h>
#include <GdiPlus.h>
#include <GL/gl.h>
#include <iostream>
#include <cmath>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "../extern/qglextensions.h"

HWND		hwnd			= NULL;
HDC			hdc				= NULL;
HGLRC		hrc				= NULL;
ULONG_PTR	gdiplusToken	= NULL;

RECT	workarea;
long	screenwidth		= 1200;
long	screenheight	= 600;
bool	active			= false;
bool	minimized		= false;

bool InitScene();
void UninitScene();
void Update(float delta);
void Render(float alpha, float elapsedtime);

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
GLuint CreateNormalizationCubemap()
{
	GLuint ret;
	unsigned char* bytes = new unsigned char[128 * 128 * 4];
	int index;
	float tmp[4];

	glGenTextures(1, &ret);
	glBindTexture(GL_TEXTURE_CUBE_MAP, ret);

	for( int i = 0; i < 128; ++i )
	{
		for( int j = 0; j < 128; ++j )
		{
			tmp[0] = 64;
			tmp[1] = 64 - (i + 0.5f);
			tmp[2] = 64 - (j + 0.5f);
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (unsigned char)(tmp[2] * 255.0f);
			bytes[index + 1] = (unsigned char)(tmp[1] * 255.0f);
			bytes[index + 0] = (unsigned char)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	for( int i = 0; i < 128; ++i )
	{
		for( int j = 0; j < 128; ++j )
		{
			tmp[0] = -64 + (j + 0.5f);
			tmp[1] = 64;
			tmp[2] = -64 + (i + 0.5f);
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (unsigned char)(tmp[2] * 255.0f);
			bytes[index + 1] = (unsigned char)(tmp[1] * 255.0f);
			bytes[index + 0] = (unsigned char)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	for( int i = 0; i < 128; ++i )
	{
		for( int j = 0; j < 128; ++j )
		{
			tmp[0] = -64 + (j + 0.5f);
			tmp[1] = 64 - (i + 0.5f);
			tmp[2] = 64;
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (unsigned char)(tmp[2] * 255.0f);
			bytes[index + 1] = (unsigned char)(tmp[1] * 255.0f);
			bytes[index + 0] = (unsigned char)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	for( int i = 0; i < 128; ++i )
	{
		for( int j = 0; j < 128; ++j )
		{
			tmp[0] = -64;
			tmp[1] = 64 - (i + 0.5f);
			tmp[2] = -64 + (j + 0.5f);
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (unsigned char)(tmp[2] * 255.0f);
			bytes[index + 1] = (unsigned char)(tmp[1] * 255.0f);
			bytes[index + 0] = (unsigned char)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	for( int i = 0; i < 128; ++i )
	{
		for( int j = 0; j < 128; ++j )
		{
			tmp[0] = -64 + (j + 0.5f);
			tmp[1] = -64;
			tmp[2] = 64 - (i + 0.5f);
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (unsigned char)(tmp[2] * 255.0f);
			bytes[index + 1] = (unsigned char)(tmp[1] * 255.0f);
			bytes[index + 0] = (unsigned char)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	for( int i = 0; i < 128; ++i )
	{
		for( int j = 0; j < 128; ++j )
		{
			tmp[0] = 64 - (j + 0.5f);
			tmp[1] = 64 - (i + 0.5f);
			tmp[2] = -64;
			tmp[3] = sqrtf(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);

			tmp[0] = (tmp[0] / tmp[3]) * 0.5f + 0.5f;
			tmp[1] = (tmp[1] / tmp[3]) * 0.5f + 0.5f;
			tmp[2] = (tmp[2] / tmp[3]) * 0.5f + 0.5f;

			index = (i * 128 + j) * 4;

			bytes[index + 3] = 255;
			bytes[index + 2] = (unsigned char)(tmp[2] * 255.0f);
			bytes[index + 1] = (unsigned char)(tmp[1] * 255.0f);
			bytes[index + 0] = (unsigned char)(tmp[0] * 255.0f);
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	delete[] bytes;

	return ret;
}
//*************************************************************************************************************
Gdiplus::Bitmap* LoadPicture(const std::wstring& file)
{
	HANDLE hFile = CreateFileW(
		file.c_str(), 
		FILE_READ_DATA,
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if( !hFile )
		return 0;

	DWORD len = GetFileSize(hFile, NULL);
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_NODISCARD, len); // leak

	if ( !hGlobal )
	{
		CloseHandle(hFile);
		return 0;
	}

	char* lpBuffer = reinterpret_cast<char*>(GlobalLock(hGlobal));
	DWORD dwBytesRead = 0;

	while( ReadFile(hFile, lpBuffer, 4096, &dwBytesRead, NULL) )
	{
		lpBuffer += dwBytesRead;

		if( dwBytesRead == 0 )
			break;

		dwBytesRead = 0;
	}

	CloseHandle(hFile);
	GlobalUnlock(hGlobal);

	IStream* pStream = NULL;

	if( CreateStreamOnHGlobal(hGlobal, FALSE, &pStream) != S_OK )
	{
		GlobalFree(hGlobal);
		return 0;
	}

	Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromStream(pStream);
	pStream->Release();

	return bitmap;
}
//*************************************************************************************************************
GLuint LoadTexture(const std::wstring& file)
{
	GLuint texid = 0;
	Gdiplus::Bitmap* bitmap = LoadPicture(file);

	if( bitmap )
	{ 
		if( bitmap->GetLastStatus() == Gdiplus::Ok )
		{
			Gdiplus::BitmapData data;
			unsigned char* tmpbuff;

			bitmap->LockBits(0, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);

			tmpbuff = new unsigned char[data.Width * data.Height * 4];
			memcpy(tmpbuff, data.Scan0, data.Width * data.Height * 4);

			for( UINT i = 0; i < data.Height; ++i )
			{
				// swap red and blue
				for( UINT j = 0; j < data.Width; ++j )
				{
					UINT index = (i * data.Width + j) * 4;
					std::swap<unsigned char>(tmpbuff[index + 0], tmpbuff[index + 2]);
				}

				// flip on X
				for( UINT j = 0; j < data.Width / 2; ++j )
				{
					UINT index1 = (i * data.Width + j) * 4;
					UINT index2 = (i * data.Width + (data.Width - j - 1)) * 4;

					std::swap<unsigned int>(*((unsigned int*)(tmpbuff + index1)), *((unsigned int*)(tmpbuff + index2)));
				}
			}

			glGenTextures(1, &texid);
			glBindTexture(GL_TEXTURE_2D, texid);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

			glBindTexture(GL_TEXTURE_2D, texid);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data.Width, data.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmpbuff);
			glBindTexture(GL_TEXTURE_2D, 0);

			GLenum err = glGetError();

			if( err != GL_NO_ERROR )
				MYERROR("Could not create texture")
			else
				std::cout << "Created texture " << data.Width << "x" << data.Height << "\n";

			bitmap->UnlockBits(&data);
			delete[] tmpbuff;
		}

		delete bitmap;
	}
	else
		MYERROR("Could not create bitmap");

	return texid;
}
//*************************************************************************************************************
GLuint LoadCubeTexture(const std::wstring& file)
{
	GLuint texid = 0;
	std::wstring path;
	std::wstring ext;

	size_t pos = file.find_last_of('.');

	if( pos == std::wstring::npos )
		return 0;

	path = file.substr(0, pos);
	ext = file.substr(pos + 1);

	std::wstring files[6] =
	{
		path,
		path,
		path,
		path,
		path,
		path
	};

	GLenum faces[] =
	{
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

	// no operator + (ya, rly...)
	files[0].append(L"_posx.");
	files[0].append(ext);

	files[1].append(L"_negx.");
	files[1].append(ext);

	files[2].append(L"_posy.");
	files[2].append(ext);

	files[3].append(L"_negy.");
	files[3].append(ext);

	files[4].append(L"_posz.");
	files[4].append(ext);

	files[5].append(L"_negz.");
	files[5].append(ext);
	
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texid);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE);

	for( int k = 0; k < 6; ++k )
	{
		Gdiplus::Bitmap* bitmap = LoadPicture(files[k]);

		if( bitmap )
		{ 
			if( bitmap->GetLastStatus() == Gdiplus::Ok )
			{
				Gdiplus::BitmapData data;
				unsigned char* tmpbuff;

				bitmap->LockBits(0, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);

				tmpbuff = new unsigned char[data.Width * data.Height * 4];
				memcpy(tmpbuff, data.Scan0, data.Width * data.Height * 4);

				for( UINT i = 0; i < data.Height; ++i )
				{
					// swap red and blue
					for( UINT j = 0; j < data.Width; ++j )
					{
						UINT index = (i * data.Width + j) * 4;
						std::swap<unsigned char>(tmpbuff[index + 0], tmpbuff[index + 2]);
					}

					// flip on X
					for( UINT j = 0; j < data.Width / 2; ++j )
					{
						UINT index1 = (i * data.Width + j) * 4;
						UINT index2 = (i * data.Width + (data.Width - j - 1)) * 4;

						std::swap<unsigned int>(*((unsigned int*)(tmpbuff + index1)), *((unsigned int*)(tmpbuff + index2)));
					}
				}

				glTexImage2D(faces[k], 0, GL_RGBA, data.Width, data.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmpbuff);

				GLenum err = glGetError();

				if( err != GL_NO_ERROR )
					MYERROR("Could not create texture")
				else
					std::cout << "Created texture " << data.Width << "x" << data.Height << "\n";

				bitmap->UnlockBits(&data);
				delete[] tmpbuff;
			}

			delete bitmap;
		}
		else
			MYERROR("Could not create bitmap");
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	return texid;
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

	case WM_ACTIVATE:
	case WM_ACTIVATEAPP:
		switch( wParam )
		{
		case WA_INACTIVE:
			if( active )
			{
				active = false;
				std::cout << "Application deactivated.\n";
			}

			break;

		case WA_ACTIVE:
		case WA_CLICKACTIVE:
			if( !active && !minimized )
			{
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
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

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

	// windowed mode
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
	Gdiplus::GdiplusShutdown(gdiplusToken);

	std::cout << "Exiting...\n";

	UnregisterClass("TestClass", wc.hInstance);
	_CrtDumpMemoryLeaks();

	return 0;
}
//*************************************************************************************************************
