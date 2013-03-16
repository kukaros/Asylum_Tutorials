//*************************************************************************************************************
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "GdiPlus.lib")

#include <iostream>
#include <d3dx9.h>
#include <GdiPlus.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#define TITLE			"Shader tutorial 15: Deferred shading"
#define MYERROR(x)		{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)	{ if( (x) ) { (x)->Release(); (x) = NULL; } }

HWND					hwnd			= NULL;
LPDIRECT3D9				direct3d		= NULL;
LPDIRECT3DDEVICE9		device			= NULL;

D3DPRESENT_PARAMETERS	d3dpp;
RECT					workarea;
ULONG_PTR				gdiplustoken;
long					screenwidth		= 800;
long					screenheight	= 600;

// must be implemented by tutorial
HRESULT InitScene();

void UninitScene();
void Update(float delta);
void Render(float alpha, float elapsedtime);
void KeyPress(WPARAM wparam);

HRESULT DXCreateEffect(const char* file, LPD3DXEFFECT* out)
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;

	if( FAILED(hr = D3DXCreateEffectFromFileA(device, file, NULL, NULL, D3DXSHADER_DEBUG, NULL, out, &errors)) )
	{
		if( errors )
		{
			char* str = (char*)errors->GetBufferPointer();
			std::cout << str << "\n\n";
		}
	}

	if( errors )
		errors->Release();

	return hr;
}
//*************************************************************************************************************
HRESULT DXGenTangentFrame(LPD3DXMESH* mesh)
{
	LPD3DXMESH newmesh = NULL;
	HRESULT hr;

	D3DVERTEXELEMENT9 decl[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 20, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 32, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0 },
		{ 0, 44, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
		D3DDECL_END()
	};

	hr = (*mesh)->CloneMesh(D3DXMESH_MANAGED, decl, device, &newmesh);

	if( FAILED(hr) )
		return hr;

	(*mesh)->Release();
	(*mesh) = NULL;

	hr = D3DXComputeTangentFrameEx(newmesh, D3DDECLUSAGE_TEXCOORD, 0,
		D3DDECLUSAGE_TANGENT, 0, D3DDECLUSAGE_BINORMAL, 0, D3DDECLUSAGE_NORMAL, 0,
		0, NULL, 0.01f, 0.25f, 0.01f, mesh, NULL);

	newmesh->Release();
	return hr;
}
//*************************************************************************************************************
void DXRenderText(const std::string& str, LPDIRECT3DTEXTURE9 tex, DWORD width, DWORD height)
{
	if( tex == 0 )
		return;

	Gdiplus::Color				outline(0xff000000);
	Gdiplus::Color				fill(0xffffffff);

	Gdiplus::Bitmap*			bitmap;
	Gdiplus::Graphics*			graphics;
	Gdiplus::GraphicsPath		path;
	Gdiplus::FontFamily			family(L"Arial");
	Gdiplus::StringFormat		format;
	Gdiplus::Pen				pen(outline, 3);
	Gdiplus::SolidBrush			brush(fill);
	std::wstring				wstr(str.begin(), str.end());

	//format.SetAlignment(Gdiplus::StringAlignmentFar);

	bitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
	graphics = new Gdiplus::Graphics(bitmap);

	// render text
	graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	graphics->SetPageUnit(Gdiplus::UnitPixel);

	path.AddString(wstr.c_str(), wstr.length(), &family, Gdiplus::FontStyleBold, 25, Gdiplus::Point(0, 0), &format);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	graphics->DrawPath(&pen, &path);
	graphics->FillPath(&brush, &path);

	// copy to texture
	Gdiplus::Rect rc(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	Gdiplus::BitmapData data;
	D3DLOCKED_RECT d3drc;

	memset(&data, 0, sizeof(Gdiplus::BitmapData));

	bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
	tex->LockRect(0, &d3drc, 0, 0);

	memcpy(d3drc.pBits, data.Scan0, width * height * 4);

	tex->UnlockRect(0);
	bitmap->UnlockBits(&data);

	delete graphics;
	delete bitmap;
}
//*************************************************************************************************************
HRESULT InitDirect3D(HWND hwnd)
{
	if( NULL == (direct3d = Direct3DCreate9(D3D_SDK_VERSION)) )
		return E_FAIL;

	d3dpp.BackBufferFormat				= D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount				= 1;
	d3dpp.BackBufferHeight				= screenheight;
	d3dpp.BackBufferWidth				= screenwidth;
	d3dpp.AutoDepthStencilFormat		= D3DFMT_D24S8;
	d3dpp.hDeviceWindow					= hwnd;
	d3dpp.Windowed						= true;
	d3dpp.EnableAutoDepthStencil		= true;
	d3dpp.PresentationInterval			= D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
	d3dpp.SwapEffect					= D3DSWAPEFFECT_DISCARD;
	d3dpp.MultiSampleType				= D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality			= 0;

	if( FAILED(direct3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &device)) )
	{
		MYERROR("Could not create Direct3D device");
		return E_FAIL;
	}

	device->SetRenderState(D3DRS_LIGHTING, false);

	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	D3DCAPS9 caps;
	device->GetDeviceCaps(&caps);

	if( caps.NumSimultaneousRTs < 2 )
	{
		MYERROR("Device does not support enough render targets");
		return E_FAIL;
	}

	return S_OK;
}
//*************************************************************************************************************
LRESULT WINAPI WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam)
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
void Adjust(tagRECT& out, long& width, long& height, DWORD style, DWORD exstyle, bool menu = false)
{
	long w = workarea.right - workarea.left;
	long h = workarea.bottom - workarea.top;

	out.left = (w - width) / 2;
	out.top = (h - height) / 2;
	out.right = (w + width) / 2;
	out.bottom = (h + height) / 2;

	AdjustWindowRectEx(&out, style, false, 0);

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
int main(int argc, char* argv[])
{
	Gdiplus::GdiplusStartupInput gdiplustartup;
	Gdiplus::GdiplusStartup(&gdiplustoken, &gdiplustartup, NULL);

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
		0L,    0L,
		GetModuleHandle(NULL),
		NULL, NULL, NULL, NULL, "TestClass", NULL
	};

	RegisterClassEx(&wc);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea, 0);

	RECT rect = { 0, 0, screenwidth, screenheight };
	DWORD style = WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	// ablakos mód
	style |= WS_SYSMENU|WS_BORDER|WS_CAPTION;
	Adjust(rect, screenwidth, screenheight, style, 0);

	hwnd = CreateWindowA("TestClass", TITLE, style,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, wc.hInstance, NULL);

	if( !hwnd )
	{
		MYERROR("Could not create window");
		goto _end;
	}

	if( FAILED(InitDirect3D(hwnd)) )
	{
		MYERROR("Failed to initialize Direct3D");
		goto _end;
	}
	
	if( FAILED(InitScene()) )
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
	UninitScene();

	if( device )
	{
		ULONG rc = device->Release();

		if( rc > 0 )
			MYERROR("You forgot to release something");
	}

	if( direct3d )
		direct3d->Release();

	Gdiplus::GdiplusShutdown(gdiplustoken);

	UnregisterClass("TestClass", wc.hInstance);
	_CrtDumpMemoryLeaks();

#ifdef _DEBUG
	system("pause");
#endif

	return 0;
}
//*************************************************************************************************************
