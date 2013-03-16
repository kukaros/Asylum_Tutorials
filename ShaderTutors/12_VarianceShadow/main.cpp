//*************************************************************************************************************
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")

#include <d3dx9.h>
#include <iostream>

#define SHADOWMAP_SIZE  512

// helper macros
#define MYERROR(x)		{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)		{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }

// external variables
extern long screenwidth;
extern long screenheight;

extern LPDIRECT3DDEVICE9				device;
extern D3DPRESENT_PARAMETERS			d3dpp;
extern LPD3DXEFFECT						variance;
extern LPD3DXEFFECT						distance;
extern LPD3DXEFFECT						specular;
extern LPD3DXEFFECT						blur;
extern LPDIRECT3DVERTEXDECLARATION9		vertexdecl;
extern LPD3DXMESH						shadowreceiver;
extern LPD3DXMESH						shadowcaster;
extern LPDIRECT3DTEXTURE9				texture1;
extern LPDIRECT3DTEXTURE9				texture2;
extern LPDIRECT3DTEXTURE9				shadowmap;
extern LPDIRECT3DTEXTURE9				blurshadowmap;

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

	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &shadowmap, NULL));
	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &blurshadowmap, NULL));
	MYVALID(device->CreateVertexDeclaration(elem, &vertexdecl));

	MYVALID(DXCreateEffect("../media/shaders/blinnphong.fx", &specular));
	MYVALID(DXCreateEffect("../media/shaders/distance.fx", &distance));
	MYVALID(DXCreateEffect("../media/shaders/variance.fx", &variance));
	MYVALID(DXCreateEffect("../media/shaders/blurshadow.fx", &blur));

	distance->SetTechnique("distance_point");

	// setup camera
	D3DXVECTOR3 eye(-3, 3, -3);
	D3DXVECTOR3 look(0, 0.5f, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)d3dpp.BackBufferWidth / (float)d3dpp.BackBufferHeight, 0.1f, 100);
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
	unsigned long flags = D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER;

	D3DXVECTOR4 lightpos(-1, 6, 5, 1);
	D3DXVECTOR3 look(0, 0.5f, 0), up(0, 1, 0);
	D3DXVECTOR4 texelsize(1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE, 0, 1);

	D3DXMATRIX inv, vp, tmp;
	D3DXMATRIX lightview, lightproj, lightvp;

	LPDIRECT3DSURFACE9 oldsurface = NULL;
	LPDIRECT3DSURFACE9 shadowsurface = NULL;

	// specular effect uniforms
	D3DXMatrixMultiply(&vp, &view, &proj);
	D3DXMatrixScaling(&world, 0.25f, 0.25f, 0.25f);

	D3DXMatrixRotationYawPitchRoll(&tmp, timeGetTime() / 1000.0f, D3DX_PI / 8, 0);
	//D3DXMatrixRotationYawPitchRoll(&tmp, D3DX_PI / 3, D3DX_PI / 8, 0);
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

	variance->SetMatrix("matWorld", &world);
	variance->SetMatrix("matViewProj", &vp);
	variance->SetMatrix("lightVP", &lightvp);
	variance->SetVector("lightPos", &lightpos);

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

			distance->SetMatrix("matWorld", &world);
			distance->CommitChanges();

			shadowreceiver->DrawSubset(0);
		}
		distance->EndPass();
		distance->End();

		// STEP 2: blur shadow map
		blurshadowmap->GetSurfaceLevel(0, &shadowsurface);

		device->SetVertexDeclaration(vertexdecl);
		device->SetRenderTarget(0, shadowsurface);
		device->SetTexture(0, shadowmap);
		device->Clear(0, NULL, flags, 0x00000000, 1.0f, 0);

		shadowsurface->Release();
		shadowsurface = NULL;

		blur->SetTechnique("blurx");
		blur->SetVector("texelsize", &texelsize);
		blur->Begin(NULL, 0);
		blur->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		blur->EndPass();
		blur->End();

		shadowmap->GetSurfaceLevel(0, &shadowsurface);

		device->SetRenderTarget(0, shadowsurface);
		device->SetTexture(0, blurshadowmap);
		device->Clear(0, NULL, flags, 0x00000000, 1.0f, 0);

		shadowsurface->Release();
		shadowsurface = NULL;

		blur->SetTechnique("blury");
		blur->SetVector("texelsize", &texelsize);
		blur->Begin(NULL, 0);
		blur->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		blur->EndPass();
		blur->End();

		// STEP 3: render scene
		device->SetRenderTarget(0, oldsurface);
		device->Clear(0, NULL, flags, 0xff6694ed, 1.0f, 0);

		oldsurface->Release();

		device->SetTexture(0, texture2);
		device->SetTexture(1, shadowmap);

		// receiver
		variance->Begin(NULL, 0);
		variance->BeginPass(0);
		{
			shadowreceiver->DrawSubset(0);
		}
		variance->EndPass();
		variance->End();

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
		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
