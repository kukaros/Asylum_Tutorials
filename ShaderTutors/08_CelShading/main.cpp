//*************************************************************************************************************
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")

#include <d3dx9.h>
#include <mmsystem.h>
#include <iostream>

#define TITLE       "Shader tutorial 8: Cel shading"
#define MYERROR(x)  { std::cout << "* Error: " << x << "!\n"; }

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

HWND							hwnd			= NULL;
LPDIRECT3D9						direct3d		= NULL;
LPDIRECT3DDEVICE9				device			= NULL;
LPD3DXMESH						mesh			= NULL;
LPD3DXEFFECT					effect			= NULL;
LPDIRECT3DTEXTURE9				texture			= NULL;
LPDIRECT3DTEXTURE9				intensity		= NULL;
LPDIRECT3DTEXTURE9				colortarget		= NULL;
LPDIRECT3DTEXTURE9				normaltarget	= NULL;
LPDIRECT3DTEXTURE9				edgetarget		= NULL;
LPDIRECT3DSURFACE9				colorsurface	= NULL;
LPDIRECT3DSURFACE9				normalsurface	= NULL;
LPDIRECT3DSURFACE9				edgesurface		= NULL;
LPDIRECT3DVERTEXDECLARATION9	vertexdecl		= NULL;

D3DPRESENT_PARAMETERS			d3dpp;
D3DXMATRIX						view, world, proj;
RECT							workarea;
long							screenwidth = 800;
long							screenheight = 600;

D3DVERTEXELEMENT9 elem[] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
	{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	D3DDECL_END()
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
HRESULT CreateColorTex(LPDIRECT3DDEVICE9 device, DWORD color, LPDIRECT3DTEXTURE9* texture);
LRESULT WINAPI WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam);

void Adjust(tagRECT& out, long& width, long& height, DWORD style, DWORD exstyle, bool menu = false);
void Render();

HRESULT InitScene()
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;

	// teapothoz más scale kell (ld. Render())
	if( FAILED(hr = D3DXLoadMeshFromXA("../media/meshes/knot.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh)) )
	{
		MYERROR("Could not load sphere");
		return hr;
	}

	if( FAILED(hr = CreateColorTex(device, 0xff77FF70, &texture)) )
	{
		MYERROR("Could not create texture");
		return hr;
	}

	if( FAILED(hr = D3DXCreateTextureFromFileA(device, "../media/textures/intensity.png", &intensity)) )
	{
		MYERROR("Could not create intensity texture");
		return hr;
	}

	if( FAILED(hr = device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &colortarget, NULL)) )
	{
		MYERROR("Could not create color target");
		return hr;
	}

	if( FAILED(hr = device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &normaltarget, NULL)) )
	{
		MYERROR("Could not create normal target");
		return hr;
	}

	if( FAILED(hr = device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &edgetarget, NULL)) )
	{
		MYERROR("Could not create edge target");
		return hr;
	}
	
	if( FAILED(device->CreateVertexDeclaration(elem, &vertexdecl)) )
	{
		MYERROR("Could not create vertex declaration");
		return hr;
	}

	edgetarget->GetSurfaceLevel(0, &edgesurface);
	colortarget->GetSurfaceLevel(0, &colorsurface);
	normaltarget->GetSurfaceLevel(0, &normalsurface);

	// érdemes debugolni a shadert (release-kor ezt vegyük ki!!)
	hr = D3DXCreateEffectFromFileA(device, "../media/shaders/celshading.fx", NULL, NULL, D3DXSHADER_DEBUG, NULL, &effect, &errors);

	// forditási hibákat kiiratjuk
	if( FAILED(hr) )
	{
		if( errors )
		{
			char* str = (char*)errors->GetBufferPointer();
			std::cout << str << "\n\n";
		}

		MYERROR("Could not create effect");
		return hr;
	}

	D3DXVECTOR3 eye(0.5f, 0.5f, -1.5f);
	D3DXVECTOR3 look(0, 0, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)d3dpp.BackBufferWidth / (float)d3dpp.BackBufferHeight, 0.1f, 10);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixIdentity(&world);

	return S_OK;
}
//*************************************************************************************************************
void Render()
{
	unsigned long flags = D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER;

	D3DXVECTOR3 axis(0, 1, 0);
	D3DXMATRIX inv;

	D3DXMatrixRotationAxis(&inv, &axis, timeGetTime() / 1000.0f);
	D3DXMatrixScaling(&world, 0.3f, 0.3f, 0.3f);
	//D3DXMatrixScaling(&world, 0.6f, 0.6f, 0.6f);
	D3DXMatrixMultiply(&world, &world, &inv);
	D3DXMatrixInverse(&inv, NULL, &world);

	device->SetTexture(0, texture);
	device->SetTexture(1, intensity);

	effect->SetMatrix("matWorld", &world);
	effect->SetMatrix("matWorldInv", &inv);
	effect->SetMatrix("matView", &view);
	effect->SetMatrix("matProj", &proj);

	LPDIRECT3DSURFACE9 backbuffer = NULL;

	device->GetRenderTarget(0, &backbuffer);
	device->SetRenderTarget(0, colorsurface);
	device->SetRenderTarget(1, normalsurface);
	device->Clear(0, NULL, flags, 0xff6694ed, 1.0f, 0);

	if( SUCCEEDED(device->BeginScene()) )
	{
		// jelenet kirajzolása normálokkal + mélységgel
		effect->SetTechnique("celshading");

		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			mesh->DrawSubset(0);
		}
		effect->EndPass();
		effect->End();

		// éldetektálás
		device->SetVertexDeclaration(vertexdecl);
		device->SetRenderTarget(0, edgesurface);
		device->SetRenderTarget(1, NULL);
		device->SetTexture(0, normaltarget);
		device->Clear(0, NULL, flags, 0xff6694ed, 1.0f, 0);

		effect->SetTechnique("edgedetect");

		D3DXVECTOR4 texelsize(1.0f / (float)screenwidth, 1.0f / (float)screenheight, 0, 1);
		effect->SetVector("texelSize", &texelsize);

		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		effect->EndPass();
		effect->End();

		// összerakás + pcf
		device->SetRenderTarget(0, backbuffer);
		device->SetTexture(0, colortarget);
		device->SetTexture(2, edgetarget);
		device->Clear(0, NULL, flags, 0xff6694ed, 1.0f, 0);

		backbuffer->Release();
		effect->SetTechnique("final");

		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		effect->EndPass();
		effect->End();

		device->EndScene();
	}

	device->SetTexture(0, NULL);
	device->SetTexture(2, NULL);

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
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &device)) )
	{
		MYERROR("Could not create Direct3D device");
		return E_FAIL;
	}

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
HRESULT CreateColorTex(LPDIRECT3DDEVICE9 device, DWORD color, LPDIRECT3DTEXTURE9* texture)
{
	UINT width = 4;
	UINT height = 4;

	D3DLOCKED_RECT rect;
	HRESULT hr = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, texture, NULL);

	if( FAILED(hr) )
		return hr;

	(*texture)->LockRect(0, &rect, NULL, 0);

	for( UINT i = 0; i < height; ++i )
	{
		for( UINT j = 0; j < width; ++j )
		{
			DWORD* ptr = ((DWORD*)rect.pBits + i * width + j);
			*ptr = color;
		}
	}

	(*texture)->UnlockRect(0);
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

	while( msg.message != WM_QUIT )
	{
		while( PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if( msg.message != WM_QUIT )
			Render();
	}

	_end:
	if( mesh )
		mesh->Release();

	if( effect )
		effect->Release();

	if( texture )
		texture->Release();

	if( intensity )
		intensity->Release();

	// surfaceket kell elöbb
	if( edgesurface )
		edgesurface->Release();

	if( colorsurface )
		colorsurface->Release();

	if( normalsurface )
		normalsurface->Release();
	
	// utána a texturákat
	if( edgetarget )
		edgetarget->Release();

	if( colortarget )
		colortarget->Release();

	if( normaltarget )
		normaltarget->Release();
	
	if( vertexdecl )
		vertexdecl->Release();

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
