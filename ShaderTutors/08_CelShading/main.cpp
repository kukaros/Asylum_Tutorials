//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>

// helper macros
#define TITLE				"Shader tutorial 8: Cel shading"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern long screenwidth;
extern long screenheight;
extern LPDIRECT3DDEVICE9 device;
extern HWND hwnd;

// tutorial variables
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
D3DXMATRIX						world, view, proj;

float vertices[36] =
{
	-0.5f, -0.5f, 0, 1, 0, 0,
	(float)screenwidth - 0.5f, -0.5f, 0, 1, 1, 0,
	-0.5f, (float)screenheight - 0.5f, 0, 1, 0, 1,

	-0.5f, (float)screenheight - 0.5f, 0, 1, 0, 1,
	(float)screenwidth - 0.5f, -0.5f, 0, 1, 1, 0,
	(float)screenwidth - 0.5f, (float)screenheight - 0.5f, 0, 1, 1, 1
};

static HRESULT CreateColorTex(LPDIRECT3DDEVICE9 device, DWORD color, LPDIRECT3DTEXTURE9* texture)
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

HRESULT InitScene()
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;

	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	SetWindowText(hwnd, TITLE);

	MYVALID(CreateColorTex(device, 0xff77FF70, &texture));
	MYVALID(D3DXLoadMeshFromXA("../media/meshes/knot.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/intensity.png", &intensity));

	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &colortarget, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &normaltarget, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &edgetarget, NULL));

	MYVALID(device->CreateVertexDeclaration(elem, &vertexdecl));

	edgetarget->GetSurfaceLevel(0, &edgesurface);
	colortarget->GetSurfaceLevel(0, &colorsurface);
	normaltarget->GetSurfaceLevel(0, &normalsurface);

	hr = D3DXCreateEffectFromFileA(device, "../media/shaders/celshading.fx", NULL, NULL, D3DXSHADER_DEBUG, NULL, &effect, &errors);

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

	D3DXVECTOR3 eye(0.5f, 0.5f, -1.5f);
	D3DXVECTOR3 look(0, 0, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)screenwidth / (float)screenheight, 0.1f, 10);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixIdentity(&world);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_RELEASE(colorsurface);
	SAFE_RELEASE(normalsurface);
	SAFE_RELEASE(edgesurface);

	SAFE_RELEASE(colortarget);
	SAFE_RELEASE(normaltarget);
	SAFE_RELEASE(edgetarget);

	SAFE_RELEASE(vertexdecl);
	SAFE_RELEASE(mesh);
	SAFE_RELEASE(effect);
	SAFE_RELEASE(texture);
	SAFE_RELEASE(intensity);
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
	static float time = 0;

	D3DXMATRIX	inv;
	D3DXVECTOR3	axis(0, 1, 0);
	D3DXVECTOR4	texelsize(1.0f / (float)screenwidth, 1.0f / (float)screenheight, 0, 1);
	
	LPDIRECT3DSURFACE9 oldtarget = NULL;

	time += elapsedtime;

	D3DXMatrixRotationAxis(&inv, &axis, time);
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

	device->GetRenderTarget(0, &oldtarget);
	device->SetRenderTarget(0, colorsurface);
	device->SetRenderTarget(1, normalsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);

	if( SUCCEEDED(device->BeginScene()) )
	{
		// draw scene + normals/depth
		effect->SetTechnique("celshading");

		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			mesh->DrawSubset(0);
		}
		effect->EndPass();
		effect->End();

		// edge detection
		device->SetVertexDeclaration(vertexdecl);
		device->SetRenderTarget(0, edgesurface);
		device->SetRenderTarget(1, NULL);
		device->SetTexture(0, normaltarget);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);

		effect->SetTechnique("edgedetect");
		effect->SetVector("texelSize", &texelsize);

		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		effect->EndPass();
		effect->End();

		// put together
		device->SetRenderTarget(0, oldtarget);
		device->SetTexture(0, colortarget);
		device->SetTexture(2, edgetarget);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);

		oldtarget->Release();
		effect->SetTechnique("final");

		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		effect->EndPass();
		effect->End();

		device->SetTexture(0, NULL);
		device->SetTexture(2, NULL);

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
