//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>

// helper macros
#define TITLE				"Shader tutorial 6: Simple water"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern long screenwidth;
extern long screenheight;
extern LPDIRECT3DDEVICE9 device;
extern HWND hwnd;

// tutorial variables
LPD3DXMESH			mesh		= NULL;
LPD3DXEFFECT		effect		= NULL;
LPDIRECT3DTEXTURE9	tex			= NULL;
LPDIRECT3DTEXTURE9	normalmap	= NULL;
D3DXMATRIX			world, view, proj;

static HRESULT CreateChecker(LPDIRECT3DDEVICE9 device, UINT xseg, UINT yseg, DWORD color1, DWORD color2, LPDIRECT3DTEXTURE9* texture)
{
	UINT width = xseg * 50;
	UINT height = xseg * 50;

	D3DLOCKED_RECT rect;
	HRESULT hr = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, texture, NULL);

	if( FAILED(hr) )
		return hr;

	(*texture)->LockRect(0, &rect, NULL, 0);

	UINT xstep = width / xseg;
	UINT ystep = height / yseg;
	UINT a, b;

	xseg = xstep * xseg;
	yseg = ystep * yseg;

	for( UINT i = 0; i < height; ++i )
	{
		for( UINT j = 0; j < width; ++j )
		{
			DWORD* ptr = ((DWORD*)rect.pBits + i * width + j);

			a = i / ystep;
			b = j / xstep;

			if( (a + b) % 2 )
				*ptr = color1;
			else
				*ptr = color2;
		}
	}

	(*texture)->UnlockRect(0);
	return S_OK;
}

HRESULT InitScene()
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;

	SetWindowText(hwnd, TITLE);

	if( FAILED(hr = D3DXLoadMeshFromXA("../media/meshes/sphere.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh)) )
	{
		MYERROR("Could not load sphere");
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

	hr = D3DXComputeTangentFrameEx(newmesh,
		D3DDECLUSAGE_TEXCOORD, 0,
		D3DDECLUSAGE_TANGENT, 0,
		D3DDECLUSAGE_BINORMAL, 0,
		D3DDECLUSAGE_NORMAL, 0,
		0, NULL, 0.01f, 0.25f, 0.01f, &mesh, NULL);

	newmesh->Release();

	if( FAILED(hr) )
	{
		MYERROR("Could not compute tangent frame");
		return hr;
	}

	if( FAILED(hr = CreateChecker(device, 10, 10, 0xff8CACFF, 0xffE2E2E2, &tex)) )
	{
		MYERROR("Could not create texture");
		return hr;
	}

	if( FAILED(hr = D3DXCreateTextureFromFileA(device, "../media/textures/wave.jpg", &normalmap)) )
	{
		MYERROR("Could not load normalmap");
		return hr;
	}

	// érdemes debugolni a shadert (release-kor ezt vegyük ki!!)
	hr = D3DXCreateEffectFromFileA(device, "../media/shaders/water.fx", NULL, NULL, D3DXSHADER_DEBUG, NULL, &effect, &errors);

	// forditási hibákat kiiratjuk
	if( FAILED(hr) )
	{
		if( errors )
		{
			char* str = (char*)errors->GetBufferPointer();
			std::cout << str << "\n\n";

			errors->Release();
		}

		MYERROR("Could not create effect");
		return hr;
	}

	D3DXVECTOR3 eye(0, 0, -1.5f);
	D3DXVECTOR3 look(0, 0, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)screenwidth / (float)screenheight, 1, 10);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixIdentity(&world);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_RELEASE(mesh);
	SAFE_RELEASE(effect);
	SAFE_RELEASE(tex);
	SAFE_RELEASE(normalmap);
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
}
//*************************************************************************************************************
void Update(float delta)
{
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	D3DXMATRIX vp, inv;

	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);
	device->SetTexture(0, tex);
	device->SetTexture(1, normalmap);

	D3DXMatrixMultiply(&vp, &view, &proj);
	D3DXMatrixInverse(&inv, NULL, &view);

	effect->SetVector("eyePos", (D3DXVECTOR4*)inv.m[3]);
	effect->SetFloat("time", (float)timeGetTime() / 1000.0f);

	D3DXMatrixInverse(&inv, NULL, &world);

	effect->SetMatrix("matWorld", &world);
	effect->SetMatrix("matWorldInv", &inv);
	effect->SetMatrix("matViewProj", &vp);

	if( SUCCEEDED(device->BeginScene()) )
	{
		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			mesh->DrawSubset(0);
		}
		effect->EndPass();
		effect->End();

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
