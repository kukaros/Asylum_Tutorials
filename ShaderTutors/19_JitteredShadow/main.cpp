//*************************************************************************************************************
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "GdiPlus.lib")

#include <d3dx9.h>
#include <iostream>

#include "../common/dxext.h"

#define SHADOWMAP_SIZE  256

// helper macros
#define MYERROR(x)		{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)		{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }

// external variables
extern long screenwidth;
extern long screenheight;

extern LPDIRECT3DDEVICE9				device;
extern D3DPRESENT_PARAMETERS			d3dpp;
extern LPD3DXEFFECT						irregularpcf;
extern LPD3DXEFFECT						distance;
extern LPD3DXEFFECT						specular;
extern LPDIRECT3DVERTEXDECLARATION9		vertexdecl;
extern LPD3DXMESH						shadowreceiver;
extern LPD3DXMESH						shadowcaster;
extern LPDIRECT3DTEXTURE9				texture1;
extern LPDIRECT3DTEXTURE9				texture2;
extern LPDIRECT3DTEXTURE9				shadowmap;
extern LPDIRECT3DTEXTURE9				noise;
extern LPDIRECT3DTEXTURE9				text;

extern short							mousedx;
extern short							mousedy;
extern short							mousedown;

// external functions
HRESULT DXCreateEffect(const char* file, LPD3DXEFFECT* out);

// tutorial variables
D3DXMATRIX view, world, proj;

float vertices[36] =
{
	-0.5f, -0.5f, 0, 1, 0, 0,
	(float)SHADOWMAP_SIZE - 0.5f, -0.5f, 0, 1, 1, 0,
	-0.5f, (float)SHADOWMAP_SIZE - 0.5f, 0, 1, 0, 1,

	-0.5f, (float)SHADOWMAP_SIZE - 0.5f, 0, 1, 0, 1,
	(float)SHADOWMAP_SIZE - 0.5f, -0.5f, 0, 1, 1, 0,
	(float)SHADOWMAP_SIZE - 0.5f, (float)SHADOWMAP_SIZE - 0.5f, 0, 1, 1, 1
};

float textvertices[36] =
{
	9.5f,			9.5f,	0, 1,	0, 0,
	521.5f,			9.5f,	0, 1,	1, 0,
	9.5f,	128.0f + 9.5f,	0, 1,	0, 1,

	9.5f,	128.0f + 9.5f,	0, 1,	0, 1,
	521.5f,			9.5f,	0, 1,	1, 0,
	521.5f,	128.0f + 9.5f,	0, 1,	1, 1
};

template <typename T>
struct state
{
	T prev;
	T curr;

	state& operator =(const T& t) {
		prev = curr = t;
		return *this;
	}

	T smooth(float alpha) {
		return prev + alpha * (curr - prev);
	}
};

state<D3DXVECTOR2> cameraangle;

HRESULT InitScene()
{
	HRESULT hr;
	
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	MYVALID(D3DXLoadMeshFromX("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &shadowreceiver));
	MYVALID(D3DXLoadMeshFromX("../media/meshes/skullocc3.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &shadowcaster));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2.jpg", &texture2));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/pcfnoise.bmp", &noise));

	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &shadowmap, NULL));
	MYVALID(device->CreateTexture(512, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));
	MYVALID(device->CreateVertexDeclaration(elem, &vertexdecl));

	MYVALID(DXCreateEffect("../media/shaders/blinnphong.fx", &specular));
	MYVALID(DXCreateEffect("../media/shaders/distance.fx", &distance));
	MYVALID(DXCreateEffect("../media/shaders/irregularpcf.fx", &irregularpcf));

	distance->SetTechnique("distance_point");

	D3DXVECTOR4 noisesize(16.0f, 16.0f, 0, 1);
	D3DXVECTOR4 texelsize(1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE, 0, 1);

	irregularpcf->SetTechnique("irregular_screen");
	irregularpcf->SetVector("noisesize", &noisesize);
	irregularpcf->SetVector("texelsize", &texelsize);

	DXRenderText("Use mouse to rotate camera\n\nButton 0: screen space\nButton 1: world space", text, 512, 128);

	// setup camera
	cameraangle = D3DXVECTOR2(0.78f, 0.78f);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)d3dpp.BackBufferWidth / (float)d3dpp.BackBufferHeight, 0.1f, 100);
	D3DXMatrixIdentity(&world);
	
	return S_OK;
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	if( wparam == 0x30 )
		irregularpcf->SetTechnique("irregular_screen");
	else if( wparam == 0x31 )
		irregularpcf->SetTechnique("irregular_light");
}
//*************************************************************************************************************
void Update(float delta)
{
	D3DXVECTOR2 cameravelocity(mousedx, mousedy);

	if( mousedown == 1 )
	{
		cameraangle.prev = cameraangle.curr;
		cameraangle.curr += cameravelocity * 0.004f;
	}

	// clamp to [-pi, pi]
	if( cameraangle.curr.y >= 1.5f )
	{
		cameraangle.curr.y = 1.5f;
		cameravelocity.y = 0;
	}

	if( cameraangle.curr.y <= -1.5f )
	{
		cameraangle.curr.y = -1.5f;
		cameravelocity.y = 0;
	}
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	unsigned long flags = D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER;

	D3DXVECTOR2 orient = cameraangle.smooth(alpha);
	D3DXVECTOR4 lightpos(-1, 6, 5, 1);
	D3DXVECTOR3 look(0, 0.5f, 0), up(0, 1, 0);
	D3DXVECTOR4 texelsize(1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE, 0, 1);

	D3DXMATRIX inv, vp, tmp;
	D3DXMATRIX lightview, lightproj, lightvp;

	LPDIRECT3DSURFACE9 oldsurface = NULL;
	LPDIRECT3DSURFACE9 shadowsurface = NULL;

	// camera
	//D3DXVECTOR3 eye(-3, 3, -3);
	D3DXVECTOR3 eye(0, 0, -5.2f);

	D3DXMatrixRotationYawPitchRoll(&world, orient.x, orient.y, 0);
	D3DXVec3TransformCoord(&eye, &eye, &world);

	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixMultiply(&vp, &view, &proj);

	// specular effect uniforms
	D3DXMatrixScaling(&world, 0.25f, 0.25f, 0.25f);
	//D3DXMatrixRotationYawPitchRoll(&tmp, timeGetTime() / 1000.0f, D3DX_PI / 8, 0);
	D3DXMatrixRotationYawPitchRoll(&tmp, D3DX_PI / 3, D3DX_PI / 8, 0);
	D3DXMatrixMultiply(&world, &world, &tmp);

	world._41 = -0.4f;
	world._42 = 0.6f;
	world._43 = 0.4f;

	D3DXMatrixInverse(&inv, NULL, &world);

	specular->SetMatrix("matWorld", &world);
	specular->SetMatrix("matWorldInv", &inv);
	specular->SetMatrix("matViewProj", &vp);

	D3DXMatrixInverse(&inv, NULL, &view);
	specular->SetVector("eyePos", (D3DXVECTOR4*)inv.m[3]);
	specular->SetVector("lightPos", &lightpos);

	// distance effect uniforms
	D3DXMatrixLookAtLH(&lightview, (D3DXVECTOR3*)&lightpos, &look, &up);
	D3DXMatrixPerspectiveFovLH(&lightproj, D3DX_PI / 4, 1, 0.1f, 20.0f);
	D3DXMatrixMultiply(&lightvp, &lightview, &lightproj);

	distance->SetMatrix("matWorld", &world);
	distance->SetMatrix("matViewProj", &lightvp);
	distance->SetVector("lightPos", &lightpos);

	// shadow receiver uniforms
	D3DXMatrixScaling(&world, 5, 0.1f, 5);

	irregularpcf->SetMatrix("matWorld", &world);
	irregularpcf->SetMatrix("matViewProj", &vp);
	irregularpcf->SetMatrix("lightVP", &lightvp);
	irregularpcf->SetVector("lightPos", &lightpos);

	if( SUCCEEDED(device->BeginScene()) )
	{
		// STEP 1: render shadow map
		shadowmap->GetSurfaceLevel(0, &shadowsurface);

		device->GetRenderTarget(0, &oldsurface);
		device->SetRenderTarget(0, shadowsurface);
		device->Clear(0, NULL, flags, 0x00000000, 1.0f, 0);

		shadowsurface->Release();
		shadowsurface = NULL;
		
		distance->Begin(NULL, 0);
		distance->BeginPass(0);
		{
			shadowcaster->DrawSubset(0);
		}
		distance->EndPass();
		distance->End();

		// STEP 3: render scene
		device->SetRenderTarget(0, oldsurface);
		device->Clear(0, NULL, flags, 0xff6694ed, 1.0f, 0);

		oldsurface->Release();

		device->SetTexture(0, texture2);
		device->SetTexture(1, shadowmap);
		device->SetTexture(2, noise);

		// receiver
		irregularpcf->Begin(NULL, 0);
		irregularpcf->BeginPass(0);
		{
			shadowreceiver->DrawSubset(0);
		}
		irregularpcf->EndPass();
		irregularpcf->End();

		// caster
		device->SetTexture(0, texture1);

		specular->Begin(NULL, 0);
		specular->BeginPass(0);
		{
			shadowcaster->DrawSubset(0);
		}
		specular->EndPass();
		specular->End();
		
		device->SetTexture(1, NULL);

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
