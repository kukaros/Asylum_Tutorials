//*************************************************************************************************************
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")

#include <d3dx9.h>
#include <iostream>

#define SHADOWMAP_SIZE		512
#define SHADOW_DOWNSAMPLE	4

// helper macros
#define MYERROR(x)		{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)		{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }

// external variables
extern long screenwidth;
extern long screenheight;

extern LPDIRECT3DDEVICE9				device;
extern D3DPRESENT_PARAMETERS			d3dpp;
extern LPD3DXEFFECT						distance;
extern LPD3DXEFFECT						rendergbuffer;
extern LPD3DXEFFECT						deferred;
extern LPD3DXEFFECT						pcf;
extern LPD3DXEFFECT						blur;
extern LPDIRECT3DVERTEXDECLARATION9		vertexdecl;
extern LPD3DXMESH						shadowreceiver;
extern LPD3DXMESH						shadowcaster;
extern LPDIRECT3DTEXTURE9				texture1;
extern LPDIRECT3DTEXTURE9				texture2;
extern LPDIRECT3DTEXTURE9				shadowmap;
extern LPDIRECT3DTEXTURE9				albedo;
extern LPDIRECT3DTEXTURE9				normaldepth;
extern LPDIRECT3DTEXTURE9				shadowscene;
extern LPDIRECT3DTEXTURE9				shadowblur;
extern LPDIRECT3DSURFACE9				albedosurf;
extern LPDIRECT3DSURFACE9				normaldepthsurf;
extern LPDIRECT3DSURFACE9				shadowsurf;
extern LPDIRECT3DSURFACE9				shadowblursurf;
extern LPDIRECT3DSURFACE9				shadowscenesurf;

// external functions
HRESULT DXCreateEffect(const char* file, LPD3DXEFFECT* out);

// tutorial variables
D3DXVECTOR4 lightpos(-1, 6, 5, 1000);
D3DXVECTOR3 look(0, 0.5f, 0), up(0, 1, 0);
D3DXVECTOR4 texelsize(1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE, 0, 1);
D3DXVECTOR4 scenesize(2.0f / (float)screenwidth, 2.0f / (float)screenheight, 0, 1);

D3DXMATRIX view, proj, viewproj;
D3DXMATRIX lightview, lightproj, lightvp;
D3DXMATRIX world[2];

float vertices[36] =
{
	-0.5f, -0.5f, 0, 1, 0, 0,
	(float)screenwidth - 0.5f, -0.5f, 0, 1, 1, 0,
	-0.5f, (float)screenheight - 0.5f, 0, 1, 0, 1,

	-0.5f, (float)screenheight - 0.5f, 0, 1, 0, 1,
	(float)screenwidth - 0.5f, -0.5f, 0, 1, 1, 0,
	(float)screenwidth - 0.5f, (float)screenheight - 0.5f, 0, 1, 1, 1
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

	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &shadowmap, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &albedo, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &normaldepth, NULL));
	MYVALID(device->CreateTexture(screenwidth / SHADOW_DOWNSAMPLE, screenheight / SHADOW_DOWNSAMPLE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &shadowscene, NULL));
	MYVALID(device->CreateTexture(screenwidth / SHADOW_DOWNSAMPLE, screenheight / SHADOW_DOWNSAMPLE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &shadowblur, NULL));
	MYVALID(device->CreateVertexDeclaration(elem, &vertexdecl));

	MYVALID(DXCreateEffect("../media/shaders/gbuffer.fx", &rendergbuffer));
	MYVALID(DXCreateEffect("../media/shaders/deferred.fx", &deferred));
	MYVALID(DXCreateEffect("../media/shaders/distance.fx", &distance));
	MYVALID(DXCreateEffect("../media/shaders/pcf.fx", &pcf));
	MYVALID(DXCreateEffect("../media/shaders/bilateralblur.fx", &blur));
  
	MYVALID(albedo->GetSurfaceLevel(0, &albedosurf));
	MYVALID(normaldepth->GetSurfaceLevel(0, &normaldepthsurf));
	MYVALID(shadowmap->GetSurfaceLevel(0, &shadowsurf));
	MYVALID(shadowscene->GetSurfaceLevel(0, &shadowscenesurf));
	MYVALID(shadowblur->GetSurfaceLevel(0, &shadowblursurf));

	deferred->SetTechnique("deferred_shadow");

	// setup camera
	D3DXVECTOR3 eye(-3, 3, -3);
	D3DXVECTOR3 look(0, 0.5f, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)d3dpp.BackBufferWidth / (float)d3dpp.BackBufferHeight, 0.1f, 100);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixMultiply(&viewproj, &view, &proj);

	rendergbuffer->SetMatrix("matViewProj", &viewproj);

	// setup light
	D3DXMatrixLookAtLH(&lightview, (D3DXVECTOR3*)&lightpos, &look, &up);
	D3DXMatrixPerspectiveFovLH(&lightproj, D3DX_PI / 4, 1, 0.1f, 20.0f);
	D3DXMatrixMultiply(&lightvp, &lightview, &lightproj);

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
	LPDIRECT3DSURFACE9 oldsurface = NULL;
	D3DXMATRIX inv, tmp;
	unsigned long flags = D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER;

	// world matrices
	D3DXMatrixScaling(&world[0], 0.25f, 0.25f, 0.25f);
	D3DXMatrixScaling(&world[1], 5, 0.1f, 5);

	D3DXMatrixRotationYawPitchRoll(&tmp, timeGetTime() / 1000.0f, D3DX_PI / 8, 0);
	D3DXMatrixMultiply(&world[0], &world[0], &tmp);
	
	world[0]._41 = -0.4f;
	world[0]._42 = 0.6f;
	world[0]._43 = 0.4f;

	// distance effect uniforms
	distance->SetMatrix("matWorld", &world[0]);
	distance->SetMatrix("lightViewProj", &lightvp);
	distance->SetVector("lightPos", &lightpos);
	
	// deferred effect uniforms
	D3DXMatrixInverse(&inv, NULL, &view);
	deferred->SetVector("eyePos", (D3DXVECTOR4*)inv.m[3]);
	deferred->SetVector("lightPos", &lightpos);

	D3DXMatrixInverse(&inv, NULL, &viewproj);
	deferred->SetMatrix("matViewProjInv", &inv);
	deferred->SetMatrix("lightViewProj", &lightvp);

	pcf->SetMatrix("matWorld", &world[1]);
	pcf->SetMatrix("matViewProj", &viewproj);
	pcf->SetMatrix("matViewProjInv", &inv);
	pcf->SetMatrix("lightViewProj", &lightvp);
	pcf->SetVector("lightPos", &lightpos);
	pcf->SetVector("texelSize", &texelsize);

	blur->SetVector("texelSize", &scenesize);

	// render
	if( SUCCEEDED(device->BeginScene()) )
	{
		// STEP 1: render shadow map
		device->GetRenderTarget(0, &oldsurface);
		device->SetRenderTarget(0, shadowsurf);
		device->Clear(0, NULL, flags, 0x00000000, 1.0f, 0);

		distance->Begin(NULL, 0);
		distance->BeginPass(0);
		{
			shadowcaster->DrawSubset(0);
		}
		distance->EndPass();
		distance->End();
		
		// STEP 2: render scene
		device->SetRenderTarget(0, albedosurf);
		device->SetRenderTarget(1, normaldepthsurf);
		device->Clear(0, NULL, flags, 0xff000000, 1.0f, 0);

		D3DXMatrixInverse(&inv, NULL, &world[0]);

		rendergbuffer->SetMatrix("matWorld", &world[0]);
		rendergbuffer->SetMatrix("matWorldInv", &inv);

		rendergbuffer->Begin(NULL, 0);
		rendergbuffer->BeginPass(0);
		{
			// caster
			device->SetTexture(0, texture1);
			shadowcaster->DrawSubset(0);

			// receiver
			D3DXMatrixInverse(&inv, NULL, &world[1]);

			rendergbuffer->SetMatrix("matWorldInv", &inv);
			rendergbuffer->SetMatrix("matWorld", &world[1]);
			rendergbuffer->CommitChanges();

			device->SetTexture(0, texture2);
			shadowreceiver->DrawSubset(0);
		}
		rendergbuffer->EndPass();
		rendergbuffer->End();
		
		// STEP 3: render shadow
		device->SetRenderTarget(0, shadowscenesurf);
		device->SetRenderTarget(1, 0);
		device->SetTexture(0, shadowmap);
		device->SetTexture(1, normaldepth);

		vertices[6] = vertices[24] = vertices[30] = screenwidth / SHADOW_DOWNSAMPLE - 0.5f;
		vertices[13] = vertices[19] = vertices[31] = screenheight / SHADOW_DOWNSAMPLE - 0.5f;

		device->SetVertexDeclaration(vertexdecl);
		device->Clear(0, NULL, flags, 0xff000000, 1.0f, 0);

		pcf->Begin(NULL, 0);
		pcf->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		pcf->EndPass();
		pcf->End();

		// STEP 4 : blur shadow
		device->SetRenderTarget(0, shadowblursurf);
		device->SetTexture(0, shadowscene);
		device->SetTexture(1, normaldepth);

		device->Clear(0, NULL, flags, 0xff000000, 1.0f, 0);

		blur->Begin(NULL, 0);
		blur->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		blur->EndPass();
		blur->End();

		// STEP 5: deferred pass
		vertices[6] = vertices[24] = vertices[30] = screenwidth - 0.5f;
		vertices[13] = vertices[19] = vertices[31] = screenheight - 0.5f;

		device->SetRenderTarget(0, oldsurface);
		device->Clear(0, NULL, flags, 0xff6694ed, 1.0f, 0);

		device->SetTexture(0, albedo);
		device->SetTexture(1, normaldepth);
		device->SetTexture(2, shadowblur);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);

		oldsurface->Release();

		deferred->Begin(NULL, 0);
		deferred->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		deferred->EndPass();
		deferred->End();

		device->SetTexture(1, NULL);
		device->SetTexture(2, NULL);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
