//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>

#include "../common/dxext.h"

#define HELP_TEXT	\
	"1 - normal mapping\n" \
	"2 - parallax mapping\n" \
	"3 - parallax occlusion mapping\n" \
	"4 - relief mapping\n"

// helper macros
#define TITLE				"Shader tutorial 7: Virtual displacement"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern long screenwidth;
extern long screenheight;
extern LPDIRECT3DDEVICE9 device;
extern HWND hwnd;

// tutorial variables
LPD3DXMESH						mesh		= NULL;
LPD3DXEFFECT					normal		= NULL;
LPD3DXEFFECT					parallax	= NULL;
LPD3DXEFFECT					occlusion	= NULL;
LPD3DXEFFECT					relief		= NULL;
LPD3DXEFFECT					effect		= NULL;
LPDIRECT3DTEXTURE9				texture		= NULL;
LPDIRECT3DTEXTURE9				normalmap	= NULL;
LPDIRECT3DTEXTURE9				text		= NULL;
LPDIRECT3DVERTEXDECLARATION9	quaddecl	= NULL;
D3DXMATRIX						world, view, proj;

float textvertices[36] =
{
	9.5f,			9.5f,	0, 1,	0, 0,
	521.5f,			9.5f,	0, 1,	1, 0,
	9.5f,	512.0f + 9.5f,	0, 1,	0, 1,

	9.5f,	512.0f + 9.5f,	0, 1,	0, 1,
	521.5f,			9.5f,	0, 1,	1, 0,
	521.5f,	512.0f + 9.5f,	0, 1,	1, 1
};

HRESULT InitScene()
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;

	SetWindowText(hwnd, TITLE);

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

	// vertex declaration for text
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	MYVALID(device->CreateVertexDeclaration(elem, &quaddecl));

	// other
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood.jpg", &texture));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/four_nh.dds", &normalmap));

	MYVALID(DXCreateEffect("../media/shaders/normal.fx", device, &normal));
	MYVALID(DXCreateEffect("../media/shaders/parallax.fx", device, &parallax));
	MYVALID(DXCreateEffect("../media/shaders/parallaxocclusion.fx", device, &occlusion));
	MYVALID(DXCreateEffect("../media/shaders/relief.fx", device, &relief));

	MYVALID(device->CreateTexture(512, 512, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));

	effect = relief;

	D3DXVECTOR3 eye(0, 0, -1.5f);
	D3DXVECTOR3 look(0, 0, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)screenwidth / (float)screenheight, 0.1f, 10);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixIdentity(&world);

	DXRenderText(HELP_TEXT, text, 512, 512);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_RELEASE(mesh);
	SAFE_RELEASE(normal);
	SAFE_RELEASE(parallax);
	SAFE_RELEASE(occlusion);
	SAFE_RELEASE(relief);
	SAFE_RELEASE(texture);
	SAFE_RELEASE(normalmap);
	SAFE_RELEASE(text);
	SAFE_RELEASE(quaddecl);
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	switch( wparam )
	{
	case 0x31:
		effect = normal;
		break;

	case 0x32:
		effect = parallax;
		break;

	case 0x33:
		effect = occlusion;
		break;

	case 0x34:
		effect = relief;
		break;

	default:
		break;
	}
}
//*************************************************************************************************************
void Update(float delta)
{
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	D3DXMATRIX vp, inv;
	D3DXVECTOR3 axis(0, 1, 0);

	time += elapsedtime;

	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);
	device->SetTexture(0, texture);
	device->SetTexture(1, normalmap);

	D3DXMatrixMultiply(&vp, &view, &proj);
	D3DXMatrixInverse(&inv, NULL, &view);

	effect->SetVector("eyePos", (D3DXVECTOR4*)inv.m[3]);

	//D3DXMatrixRotationYawPitchRoll(&world, 2000.0f, 2000.0f, 3000.0f);
	D3DXMatrixRotationYawPitchRoll(&world, time * 0.2f, time * 0.2f, time * 0.3f);
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

		// render text
		device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		device->SetTexture(0, text);
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, textvertices, 6 * sizeof(float));

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		device->SetRenderState(D3DRS_ZENABLE, TRUE);

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
