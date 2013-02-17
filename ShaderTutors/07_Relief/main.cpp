//*************************************************************************************************************
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")

#include <d3dx9.h>
#include <mmsystem.h>
#include <iostream>

#define TITLE		"Shader tutorial 7: Relief mapping"
#define MYERROR(x)	{ std::cout << "* Error: " << x << "!\n"; }

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

HWND						hwnd			= NULL;
LPDIRECT3D9					direct3d		= NULL;
LPDIRECT3DDEVICE9			device			= NULL;
LPD3DXMESH					mesh			= NULL;
LPD3DXEFFECT				effect			= NULL;
LPDIRECT3DTEXTURE9			texture			= NULL;
LPDIRECT3DTEXTURE9			normalmap		= NULL;

D3DPRESENT_PARAMETERS		d3dpp;
D3DXMATRIX					view, world, proj, vp;
RECT						workarea;
long						screenwidth = 800;
long						screenheight = 600;

HRESULT InitDirect3D(HWND hwnd);
HRESULT InitScene();
LRESULT WINAPI WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam);

void Adjust(tagRECT& out, long& width, long& height, DWORD style, DWORD exstyle, bool menu = false);
void Render();

HRESULT InitScene()
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;

	if( FAILED(hr = D3DXLoadMeshFromXA("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh)) )
	{
		MYERROR("Could not load mesh");
		return hr;
	}

	// generate tangent frame
	LPD3DXMESH newmesh = NULL;

	D3DVERTEXELEMENT9 decl[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 20, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 32, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0 },
		{ 0, 44, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
		D3DDECL_END()
	};

	if( FAILED(hr = mesh->CloneMesh(D3DXMESH_MANAGED, decl, device, &newmesh)) )
	{
		MYERROR("Could not clone mesh");
		return hr;
	}

	mesh->Release();
	mesh = NULL;

	hr = D3DXComputeTangentFrameEx(newmesh, D3DDECLUSAGE_TEXCOORD, 0,
		D3DDECLUSAGE_TANGENT, 0, D3DDECLUSAGE_BINORMAL, 0, D3DDECLUSAGE_NORMAL, 0,
		0, NULL, 0.01f, 0.25f, 0.01f, &mesh, NULL);

	newmesh->Release();

	if( FAILED(hr) )
	{
		MYERROR("Could not compute tangent frame");
		return hr;
	}

	if( FAILED(hr = D3DXCreateTextureFromFileA(device, "../media/textures/wood.jpg", &texture)) )
	{
		MYERROR("Could not create texture");
		return hr;
	}

	if( FAILED(hr = D3DXCreateTextureFromFileA(device, "../media/textures/four_nh.dds", &normalmap)) )
	{
		MYERROR("Could not load normalmap");
		return hr;
	}
	
	// érdemes debugolni a shadert (release-kor ezt vegyük ki!!)
	hr = D3DXCreateEffectFromFileA(device, "../media/shaders/relief.fx", NULL, NULL, D3DXSHADER_DEBUG, NULL, &effect, &errors);

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

	D3DXVECTOR3 eye(0, 0, -1.5f);
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
	device->Clear(0, NULL, flags, 0xff6694ed, 1.0f, 0);

	D3DXVECTOR3 axis(0, 1, 0);
	//D3DXMatrixRotationYawPitchRoll(&world, 2000.0f, 2000.0f, 3000.0f);
	D3DXMatrixRotationYawPitchRoll(&world, timeGetTime() / 2000.0f, timeGetTime() / 4000.0f, timeGetTime() / 3000.0f);

	D3DXMATRIX inv;

	// vagy átadod az eye vektort
	D3DXMatrixInverse(&inv, NULL, &view);
	effect->SetVector("eyePos", (D3DXVECTOR4*)inv.m[3]);

	D3DXMatrixMultiply(&vp, &view, &proj);
	D3DXMatrixInverse(&inv, NULL, &world);

	device->SetTexture(0, texture);
	device->SetTexture(1, normalmap);

	effect->SetMatrix("matWorld", &world);
	effect->SetMatrix("matWorldInv", &inv);
	effect->SetMatrix("matViewProj", &vp);

	if( SUCCEEDED(device->BeginScene()) )
	{
		// normál esetben elkérjük töle a passok számát
		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			// itt meg a subsetek számát...
			mesh->DrawSubset(0);
		}
		effect->EndPass();
		effect->End();

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
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &device)) )
	{
		MYERROR("Could not create Direct3D device");
		return E_FAIL;
	}

	D3DCAPS9 caps;
	device->GetDeviceCaps(&caps);
	
	if( caps.VertexShaderVersion < D3DVS_VERSION(3, 0) || caps.PixelShaderVersion < D3DPS_VERSION(3, 0) )
	{
		MYERROR("Device does not support Shader Model 3.0");
		return E_FAIL;
	}

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

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
		0L,
		0L,
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

	if( normalmap )
		normalmap->Release();

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
