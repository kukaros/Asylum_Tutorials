//*************************************************************************************************************
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")

#include <d3dx9.h>
#include <iostream>
#include <list>

#define TITLE		"Tutorial 16: Path tracing"
#define MYERROR(x)  { std::cout << "* Error: " << x << "!\n"; }

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#include <ctime>

#include "pathtracer.h"

class TraceHelper;

HWND						hwnd			= NULL;
LPDIRECT3D9					direct3d		= NULL;
LPDIRECT3DDEVICE9			device			= NULL;
LPDIRECT3DTEXTURE9			texture			= NULL;

D3DPRESENT_PARAMETERS		d3dpp;
D3DXMATRIX					view, world, proj;
RECT						workarea;
long						screenwidth = 800;
long						screenheight = 600;
PathTracer*					tracer;
TraceHelper*				helper;
Thread*						helperthread;

class TraceHelper
{
public:
	void Run() {
		tracer->Render(view, proj);
	}
};

float vertices[36] =
{
	-0.5f, -0.5f, 0, 1, 0, 0,
	(float)screenwidth - 0.5f, -0.5f, 0, 1, 1, 0,
	-0.5f, (float)screenheight - 0.5f, 0, 1, 0, 1,

	-0.5f, (float)screenheight - 0.5f, 0, 1, 0, 1,
	(float)screenwidth - 0.5f, -0.5f, 0, 1, 1, 0,
	(float)screenwidth - 0.5f, (float)screenheight - 0.5f, 0, 1, 1, 1
};

HRESULT InitDirect3D(HWND hwnd);
HRESULT InitScene();
LRESULT WINAPI WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam);

void Adjust(tagRECT& out, long& width, long& height, DWORD style, DWORD exstyle, bool menu = false);
void Render(D3DXVECTOR4* accum, UINT samplesdone, const RECT& rc);

HRESULT InitScene()
{
	srand((unsigned int)time(NULL));

	if( FAILED(device->CreateTexture(d3dpp.BackBufferWidth, d3dpp.BackBufferHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, NULL)) )
	{
		MYERROR("Could not create texture");
		return E_FAIL;
	}

	tracer = new PathTracer();

	// sphere 1
	Sphere* s = new Sphere(D3DXVECTOR3(2, -2.5f, 1), 2);

	s->Material.Color = D3DXCOLOR(1, 1, 1, 1);
	s->Material.RefractiveIndex = 1.4f;

	tracer->AddObject(s);

	// sphere 2
	s = new Sphere(D3DXVECTOR3(-2, -2.5f, -1), 2);

	s->Material.Color = D3DXCOLOR(1, 1, 1, 1);
	s->Material.RefractiveIndex = 1.4f;

	tracer->AddObject(s);

	// light
	Box* b = new Box(D3DXVECTOR3(0, 4.95f, 0), D3DXVECTOR3(2, 1, 2));

	b->Material.Color = D3DXCOLOR(1, 1, 0.6f, 1);
	b->Material.Emittance = D3DXVECTOR3(30, 30, 30);

	tracer->AddObject(b);

	// back
	b = new Box(D3DXVECTOR3(0, 0, 5), D3DXVECTOR3(10, 10, 1));
	b->Material.Color = D3DXCOLOR(1, 1, 1, 1);

	tracer->AddObject(b);

	// left
	b = new Box(D3DXVECTOR3(-5, 0, 0), D3DXVECTOR3(1, 10, 10));
	b->Material.Color = D3DXCOLOR(1, 0.3f, 0.1f, 1);

	tracer->AddObject(b);

	// right
	b = new Box(D3DXVECTOR3(5, 0, 0), D3DXVECTOR3(1, 10, 10));
	b->Material.Color = D3DXCOLOR(0.3f, 1, 0.1f, 1);

	tracer->AddObject(b);

	// top
	b = new Box(D3DXVECTOR3(0, 5, 0), D3DXVECTOR3(10, 1, 10));
	b->Material.Color = D3DXCOLOR(1, 1, 1, 1);

	tracer->AddObject(b);

	// bottom
	b = new Box(D3DXVECTOR3(0, -5, 0), D3DXVECTOR3(10, 1, 10));
	b->Material.Color = D3DXCOLOR(1, 1, 1, 1);

	tracer->AddObject(b);

	// camera
	D3DXVECTOR3 eye(0, 1.5f, -15);
	D3DXVECTOR3 look(0, -0.1f, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)d3dpp.BackBufferWidth / (float)d3dpp.BackBufferHeight, 1, 10);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixIdentity(&world);

	tracer->Width = d3dpp.BackBufferWidth;
	tracer->Height = d3dpp.BackBufferHeight;
	tracer->NumSamples = 1000;

	D3DLOCKED_RECT rect;

	if( SUCCEEDED(texture->LockRect(0, &rect, NULL, 0)) )
	{
		memset(rect.pBits, 0, tracer->Width * tracer->Height * sizeof(DWORD));
		texture->UnlockRect(0);
	}

	tracer->present = &Render;
	return S_OK;
}
//*************************************************************************************************************
void Render(D3DXVECTOR4* accum, UINT samplesdone, const RECT& rc)
{
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_LIGHTING, FALSE);

	if( samplesdone <= tracer->NumSamples )
	{
		D3DLOCKED_RECT	rect;
		D3DXCOLOR		tmp1;

		if( SUCCEEDED(texture->LockRect(0, &rect, &rc, 0)) )
		{
			for( LONG i = 0; i < rc.bottom - rc.top; ++i )
			{
				for( LONG j = 0; j < rc.right - rc.left; ++j )
				{
					DWORD& dest = *((DWORD*)rect.pBits + i * tracer->Width + j);
					D3DXVECTOR4& acc = accum[(rc.top + i) * tracer->Width + rc.left + j];

					tmp1.r = acc.x / samplesdone;
					tmp1.g = acc.y / samplesdone;
					tmp1.b = acc.z / samplesdone;

					dest = tmp1;
				}
			}

			texture->UnlockRect(0);
		}
	}

	float linevertices[] =
	{
		(float)rc.left, (float)rc.top, 0, 1,
		(float)rc.right, (float)rc.top, 0, 1,

		(float)rc.right, (float)rc.top, 0, 1,
		(float)rc.right, (float)rc.bottom, 0, 1,

		(float)rc.right, (float)rc.bottom, 0, 1,
		(float)rc.left, (float)rc.bottom, 0, 1,

		(float)rc.left, (float)rc.bottom, 0, 1,
		(float)rc.left, (float)rc.top, 0, 1
	};

	if( SUCCEEDED(device->BeginScene()) )
	{
		device->SetTexture(0, texture);
		device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));

		device->SetTexture(0, 0);
		device->SetFVF(D3DFVF_XYZRHW);
		device->DrawPrimitiveUP(D3DPT_LINELIST, 4, linevertices, sizeof(D3DXVECTOR4));

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
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
		D3DCREATE_MULTITHREADED|D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &device)) )
	{
		MYERROR("Could not create Direct3D device");
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
		tracer->Kill();

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
//*************************************************************************************************************
void Adjust(tagRECT& out, long& width, long& height, DWORD style, DWORD exstyle, bool menu)
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
	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		(WNDPROC)WndProc,
		0L,
		0L,
		GetModuleHandle(NULL),
		NULL, NULL, NULL, NULL,
		"TestClass", NULL
	};

	RegisterClassEx(&wc);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea, 0);

	MSG msg;
	RECT rect = { 0, 0, screenwidth, screenheight };
	DWORD style = WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	// windowed mode
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
	
	ZeroMemory(&msg, sizeof(msg));

	rect.left = rect.top = 0;
	rect.right = screenwidth;
	rect.bottom = screenheight;

	helper = new TraceHelper();
	helperthread = new Thread();

	helperthread->Attach<TraceHelper>(helper, &TraceHelper::Run);
	helperthread->Start();

	while( msg.message != WM_QUIT )
	{
		while( PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if( msg.message != WM_QUIT && tracer->Finished() )
			Render(0, tracer->NumSamples + 1, rect);
	}

	helperthread->Close();

	delete helperthread;
	delete helper;

_end:
	delete tracer;

	if( texture )
		texture->Release();

	if( device )
	{
		ULONG rc = device->Release();

		if( rc > 0 )
			MYERROR("You forgot to release something");
	}

	if( direct3d )
		direct3d->Release();

	UnregisterClass("TestClass", wc.hInstance);
	_CrtDumpMemoryLeaks();

#ifdef _DEBUG
	system("pause");
#endif

	return 0;
}
//*************************************************************************************************************
