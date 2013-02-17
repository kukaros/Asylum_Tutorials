//*************************************************************************************************************
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#ifdef _DEBUG
#	if _MSC_VER == 1700
#		pragma comment(lib, "vorbis_vc110d.lib")
#	elif _MSC_VER == 1600
#		pragma comment(lib, "vorbis_vc100d.lib")
#	endif
#else
#	if _MSC_VER == 1700
#		pragma comment(lib, "vorbis_vc110.lib")
#	elif _MSC_VER == 1600
#		pragma comment(lib, "vorbis_vc100.lib")
#	endif
#endif

#include <d3dx9.h>
#include <iostream>
#include <ctime>

#include "particlesystem.h"
#include "audiostreamer.h"

#define TITLE		"Shader tutorial 11: Particle systems & audio streaming"
#define MYERROR(x)	{ std::cout << "* Error: " << x << "!\n"; }

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

HWND						hwnd			= NULL;
LPDIRECT3D9					direct3d		= NULL;
LPDIRECT3DDEVICE9			device			= NULL;
LPD3DXEFFECT				effect			= NULL;
LPD3DXMESH					mesh			= NULL;
LPDIRECT3DTEXTURE9			texture1		= NULL;
LPDIRECT3DTEXTURE9			texture2		= NULL;

IXAudio2*					xaudio2			= NULL;
IXAudio2MasteringVoice*		masteringvoice	= NULL;
Sound*						firesound;
Sound*						music;
AudioStreamer				streamer;
Thread						worker;

D3DPRESENT_PARAMETERS		d3dpp;
D3DXMATRIX					view, world, proj;
RECT						workarea;
long						screenwidth = 800;
long						screenheight = 600;

ParticleSystem				system1;

HRESULT InitDirect3D(HWND hwnd);
HRESULT InitXAudio2();
HRESULT InitScene();
LRESULT WINAPI WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam);

void Adjust(tagRECT& out, long& width, long& height, DWORD style, DWORD exstyle, bool menu = false);
void Update(float delta);
void Render(float alpha, float elapsedtime);

HRESULT InitScene()
{
	HRESULT hr;
	
	if( FAILED(hr = D3DXLoadMeshFromX("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh)) )
	{
		MYERROR("Could not create box");
		return hr;
	}

	if( FAILED(hr = D3DXCreateTextureFromFileA(device, "../media/textures/fire.png", &texture1)) )
	{
		MYERROR("Could not create texture");
		return hr;
	}

	if( FAILED(hr = D3DXCreateTextureFromFileA(device, "../media/textures/stones.jpg", &texture2)) )
	{
		MYERROR("Could not create texture");
		return hr;
	}

	system1.Initialize(device, 500);
	system1.ParticleTexture = texture1;

	// load fire sound
	firesound = streamer.LoadSound(xaudio2, "../media/sound/fire.ogg");

	// create streaming thread and load music
	worker.Attach<AudioStreamer>(&streamer, &AudioStreamer::Update);
	worker.Start();

	music = streamer.LoadSoundStream(xaudio2, "../media/sound/angelica.ogg");

	// setup camera
	D3DXVECTOR3 eye(-3, 3, -3);
	D3DXVECTOR3 look(0, 0.5f, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)d3dpp.BackBufferWidth / (float)d3dpp.BackBufferHeight, 0.1f, 100);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixIdentity(&world);

	// start playing sounds
	music->GetVoice()->SetVolume(4);
	firesound->GetVoice()->SetVolume(0.7f);
	
	firesound->Play();
	music->Play();

	return S_OK;
}
//*************************************************************************************************************
void Update(float delta)
{
	system1.Update();
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	unsigned long flags = D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER;
	device->Clear(0, NULL, flags, 0xff6694ed, 1.0f, 0);

	device->SetTransform(D3DTS_VIEW, &view);
	device->SetTransform(D3DTS_PROJECTION, &proj);

	if( SUCCEEDED(device->BeginScene()) )
	{
		D3DXMatrixScaling(&world, 5, 0.1f, 5);
		device->SetTransform(D3DTS_WORLD, &world);

		device->SetTexture(0, texture2);
		mesh->DrawSubset(0);

		D3DXMatrixIdentity(&world);
		device->SetTransform(D3DTS_WORLD, &world);

		system1.Draw(world, view);

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

	device->SetRenderState(D3DRS_LIGHTING, false);

	return S_OK;
}
//*************************************************************************************************************
HRESULT InitXAudio2()
{
	HRESULT hr;

	if( FAILED(hr = XAudio2Create(&xaudio2, XAUDIO2_DEBUG_ENGINE)) )
	{
		MYERROR("Could not create XAudio2 object");
		return E_FAIL;
	}

	if( FAILED(hr = xaudio2->CreateMasteringVoice(&masteringvoice)) )
	{
		MYERROR("Could not create mastering voice");
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

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

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

	if( FAILED(InitXAudio2()) )
	{
		MYERROR("Failed to initialize XAudio2");
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
	streamer.Destroy();
	WaitForSingleObject(worker.GetHandle(), 3000);

	worker.Close();

	if( masteringvoice )
		masteringvoice->DestroyVoice();

	if( xaudio2 )
		xaudio2->Release();

	// ezt se!
	system1.~ParticleSystem();

	if( mesh )
		mesh->Release();

	if( effect )
		effect->Release();

	if( texture1 )
		texture1->Release();

	if( texture2 )
		texture2->Release();

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

	CoUninitialize();

#ifdef _DEBUG
	system("pause");
#endif

	return 0;
}
//*************************************************************************************************************
