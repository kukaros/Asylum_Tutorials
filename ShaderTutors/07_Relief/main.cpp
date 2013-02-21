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

// helper macros
#define MYERROR(x)		{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)		{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }

// external variables
extern long screenwidth;
extern long screenheight;

extern LPDIRECT3DDEVICE9		device;
extern D3DPRESENT_PARAMETERS	d3dpp;
extern LPD3DXMESH				mesh;
extern LPD3DXEFFECT				effect;
extern LPDIRECT3DTEXTURE9		texture;
extern LPDIRECT3DTEXTURE9		normalmap;

// external functions
HRESULT DXCreateEffect(const char* file, LPD3DXEFFECT* out);

// tutorial variables
D3DXMATRIX view, world, proj, vp;

HRESULT InitDirect3D(HWND hwnd);
HRESULT InitScene();
LRESULT WINAPI WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam);

void Adjust(tagRECT& out, long& width, long& height, DWORD style, DWORD exstyle, bool menu = false);
void Render();

HRESULT InitScene()
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;

	MYVALID(D3DXLoadMeshFromXA("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh));

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

	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood.jpg", &texture));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/four_nh.dds", &normalmap));

	MYVALID(DXCreateEffect("../media/shaders/relief.fx", &effect));

	D3DXVECTOR3 eye(0, 0, -1.5f);
	D3DXVECTOR3 look(0, 0, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)d3dpp.BackBufferWidth / (float)d3dpp.BackBufferHeight, 0.1f, 10);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixIdentity(&world);

	return S_OK;
}
//*************************************************************************************************************
void Update(float delta)
{
	// do nothing
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	unsigned long flags = D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER;
	device->Clear(0, NULL, flags, 0xff6694ed, 1.0f, 0);

	D3DXVECTOR3 axis(0, 1, 0);
	D3DXMATRIX inv;

	//D3DXMatrixRotationYawPitchRoll(&world, 2000.0f, 2000.0f, 3000.0f);
	D3DXMatrixRotationYawPitchRoll(&world, time * 0.2f, time * 0.2f, time * 0.3f);

	time += elapsedtime;

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
