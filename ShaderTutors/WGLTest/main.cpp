//*************************************************************************************************************
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "GdiPlus.lib")

#include <Windows.h>
#include <GdiPlus.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>

// helper macros
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

// external variables
extern HDC hdc;
extern long screenwidth;
extern long screenheight;

// tutorial variables
GLuint texid = 0;

struct CommonVertex
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};

float vertices[24 * sizeof(CommonVertex)] =
{
	-0.5f, -0.5f, 0.5f, 0, -1, 0, 1, 0,
	-0.5f, -0.5f, -0.5f, 0, -1, 0, 1, 1,
	0.5f, -0.5f, -0.5f, 0, -1, 0, 0, 1,
	0.5f, -0.5f, 0.5f, 0, -1, 0, 0, 0,

	-0.5f, 0.5f, 0.5f, 0, 1, 0, 0, 0,
	0.5f, 0.5f, 0.5f, 0, 1, 0, 1, 0,
	0.5f, 0.5f, -0.5f, 0, 1, 0, 1, 1,
	-0.5f, 0.5f, -0.5f, 0, 1, 0, 0, 1,

	-0.5f, -0.5f, 0.5f, 0, 0, 1, 0, 0,
	0.5f, -0.5f, 0.5f, 0, 0, 1, 1, 0,
	0.5f, 0.5f, 0.5f, 0, 0, 1, 1, 1,
	-0.5f, 0.5f, 0.5f, 0, 0, 1, 0, 1,

	0.5f, -0.5f, 0.5f, 1, 0, 0, 0, 0,
	0.5f, -0.5f, -0.5f, 1, 0, 0, 1, 0,
	0.5f, 0.5f, -0.5f, 1, 0, 0, 1, 1,
	0.5f, 0.5f, 0.5f, 1, 0, 0, 0, 1,

	0.5f, -0.5f, -0.5f, 0, 0, -1, 0, 0,
	-0.5f, -0.5f, -0.5f, 0, 0, -1, 1, 0,
	-0.5f, 0.5f, -0.5f, 0, 0, -1, 1, 1,
	0.5f, 0.5f, -0.5f, 0, 0, -1, 0, 1,

	-0.5f, -0.5f, -0.5f, -1, 0, 0, 0, 0,
	-0.5f, -0.5f, 0.5f, -1, 0, 0, 1, 0,
	-0.5f, 0.5f, 0.5f, -1, 0, 0, 1, 1,
	-0.5f, 0.5f, -0.5f, -1, 0, 0, 0, 1
};

unsigned short indices[36] =
{
	0, 1, 2, 2, 3, 0, 
	4, 5, 6, 6, 7, 4,
	8, 9, 10, 10, 11, 8,
	12, 13, 14, 14, 15, 12,
	16, 17, 18, 18, 19, 16,
	20, 21, 22, 22, 23, 20
};

bool InitScene()
{
	// setup opengl
	//glClearColor(0.4f, 0.58f, 0.93f, 1.0f);
	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glShadeModel(GL_SMOOTH);

	glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0f);
	glLightModeli(0x81F8, 0x81FA);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_TEXTURE_2D);

	float spec[] = { 1, 1, 1, 1 };

	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);

	return true;
}
//*************************************************************************************************************
void LoadTexture(const std::wstring& file)
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
		return;

	DWORD len = GetFileSize(hFile, NULL);
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_NODISCARD, len);

	if ( !hGlobal )
	{
		CloseHandle(hFile);
		return;
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
		return;
	}

	ULONG_PTR gdiplusToken;

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromStream(pStream);
	pStream->Release();

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
				for( UINT j = 0; j < data.Width; ++j )
				{
					UINT index = (i * data.Width + j) * 4;
					std::swap<unsigned char>(tmpbuff[index + 0], tmpbuff[index + 2]);
				}
			}

			if( texid == 0 )
			{
				glGenTextures(1, &texid);
				glBindTexture(GL_TEXTURE_2D, texid);

				glTexParameteri(GL_TEXTURE_2D, 0x8191, GL_TRUE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

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

	Gdiplus::GdiplusShutdown(gdiplusToken);
	GlobalFree(hGlobal);
}
//*************************************************************************************************************
void UninitScene()
{
	if( texid != 0 )
		glDeleteTextures(1, &texid);

	texid = 0;
}
//*************************************************************************************************************
void Update(float delta)
{
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	float* vert = &vertices[0];

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, 2, 0, 0, 0, 0, 1, 0);

	float pos[] = { 6, 3, 10, 1 };
	glLightfv(GL_LIGHT0, GL_POSITION, pos);

	glRotatef(fmodf(time * 20.0f, 360.0f), 1, 0, 0);
	glRotatef(fmodf(time * 20.0f, 360.0f), 0, 1, 0);

	time += elapsedtime;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (float)screenwidth / (float)screenheight, 0.1, 100.0);

	// draw cube
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(CommonVertex), vert);
	glTexCoordPointer(2, GL_FLOAT, sizeof(CommonVertex), vert + 6);
	glNormalPointer(GL_FLOAT, sizeof(CommonVertex), vert + 3);

	glBindTexture(GL_TEXTURE_2D, texid);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indices);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_BLEND);

	SwapBuffers(hdc);
}
//*************************************************************************************************************
