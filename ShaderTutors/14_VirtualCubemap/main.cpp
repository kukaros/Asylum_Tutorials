//*************************************************************************************************************
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")

#include <d3dx9.h>
#include <iostream>

#define SHADOWMAP_SIZE	512

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
extern LPDIRECT3DCUBETEXTURE9			virtualcube;
extern LPDIRECT3DSURFACE9				depthstencil;
extern LPDIRECT3DSURFACE9				shadowsurf;

// external functions
void CreateCubeMatrices(D3DXMATRIX out[6], const D3DXVECTOR3& eyepos);
void DebugVisualize(LPDIRECT3DBASETEXTURE9 tex, float width, float height);
void FillCubeFace(LPDIRECT3DCUBETEXTURE9 tex, D3DCUBEMAP_FACES face, UINT facelength, const D3DXVECTOR4& coords);

HRESULT DXCreateEffect(const char* file, LPD3DXEFFECT* out);

// tutorial variables
D3DXVECTOR4 lightpos(0, 0.6f, 0, 1);
D3DXVECTOR4 texelsize(1.0f / (SHADOWMAP_SIZE * 4), 1.0f / (SHADOWMAP_SIZE * 4), 0, 1);

D3DXMATRIX view, proj, world[3], inv[3];
D3DXMATRIX world2[3];
D3DXMATRIX lightview[6], lightproj;
D3DVIEWPORT9 viewport;

float vertices[36] =
{
	-0.5f, -0.5f, 0, 1, 0, 0,
	(float)SHADOWMAP_SIZE * 4 - 0.5f, -0.5f, 0, 1, 1, 0,
	-0.5f, (float)SHADOWMAP_SIZE * 4 - 0.5f, 0, 1, 0, 1,

	-0.5f, (float)SHADOWMAP_SIZE * 4 - 0.5f, 0, 1, 0, 1,
	(float)SHADOWMAP_SIZE * 4 - 0.5f, -0.5f, 0, 1, 1, 0,
	(float)SHADOWMAP_SIZE * 4 - 0.5f, (float)SHADOWMAP_SIZE * 4 - 0.5f, 0, 1, 1, 1
};

// tutorial functions
void BlurShadowmap(const char* technique, LPDIRECT3DTEXTURE9 src, LPDIRECT3DTEXTURE9 dest);

HRESULT InitScene()
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;
	UINT vcubelength = SHADOWMAP_SIZE / 2;

	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	MYVALID(D3DXLoadMeshFromX("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &shadowreceiver));
	MYVALID(D3DXLoadMeshFromX("../media/meshes/lamp.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &shadowcaster));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2.jpg", &texture2));
	
	MYVALID(device->CreateTexture(SHADOWMAP_SIZE * 4, SHADOWMAP_SIZE * 4, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &shadowmap, NULL));
	MYVALID(device->CreateTexture(SHADOWMAP_SIZE * 4, SHADOWMAP_SIZE * 4, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &blurshadowmap, NULL));
	MYVALID(device->CreateDepthStencilSurface(SHADOWMAP_SIZE * 4, SHADOWMAP_SIZE * 4, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, false, &depthstencil, NULL));
	MYVALID(device->CreateCubeTexture(vcubelength, 1, 0, D3DFMT_G32R32F, D3DPOOL_MANAGED, &virtualcube, NULL));
	MYVALID(device->CreateVertexDeclaration(elem, &vertexdecl));

	MYVALID(DXCreateEffect("../media/shaders/blinnphong.fx", &specular));
	MYVALID(DXCreateEffect("../media/shaders/distance.fx", &distance));
	MYVALID(DXCreateEffect("../media/shaders/vcvariance.fx", &variance));
	MYVALID(DXCreateEffect("../media/shaders/blurshadow.fx", &blur));

	MYVALID(shadowmap->GetSurfaceLevel(0, &shadowsurf));

	// setup virtual cubemap
	FillCubeFace(virtualcube, D3DCUBEMAP_FACE_POSITIVE_X, vcubelength, D3DXVECTOR4(0.75f, 0.5f, 0.25f, 0.5f));
	FillCubeFace(virtualcube, D3DCUBEMAP_FACE_NEGATIVE_X, vcubelength, D3DXVECTOR4(0.25f, 0.0f, 0.25f, 0.5f));
	FillCubeFace(virtualcube, D3DCUBEMAP_FACE_POSITIVE_Y, vcubelength, D3DXVECTOR4(0.25f, 0.5f, 0.25f, 0.0f));
	FillCubeFace(virtualcube, D3DCUBEMAP_FACE_NEGATIVE_Y, vcubelength, D3DXVECTOR4(0.25f, 0.5f, 0.75f, 0.5f));
	FillCubeFace(virtualcube, D3DCUBEMAP_FACE_POSITIVE_Z, vcubelength, D3DXVECTOR4(1.0f, 0.75f, 0.25f, 0.5f));
	FillCubeFace(virtualcube, D3DCUBEMAP_FACE_NEGATIVE_Z, vcubelength, D3DXVECTOR4(0.5f, 0.25f, 0.25f, 0.5f));

	// setup camera
	D3DXVECTOR3 eye(-3, 3, -3);
	D3DXVECTOR3 look(0, 0.5f, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)d3dpp.BackBufferWidth / (float)d3dpp.BackBufferHeight, 0.1f, 100);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);

	CreateCubeMatrices(lightview, (D3DXVECTOR3&)lightpos);

	viewport.MinZ = 0.0f;
	viewport.MaxZ = 1.0f;
	
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
	LPDIRECT3DSURFACE9 olddepthstencil = NULL;

	D3DXMATRIX tmp, invview;
	D3DXMATRIX vp, lightvp;

	unsigned long flags = D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER;
	float angle = fmodf(timeGetTime() / 1000.0f, D3DX_PI * 2.0f);

	// object transformations
	D3DXMatrixScaling(&world[0], 0.7f, 0.7f, 0.7f);
	D3DXMatrixScaling(&world[1], 0.7f, 0.7f, 0.7f);
	D3DXMatrixScaling(&world[2], 0.7f, 0.7f, 0.7f);
	D3DXMatrixRotationY(&tmp, angle);

	world[0]._41 = -1.0f;  world[1]._41 = 0.5f;  world[2]._41 = 0.5f;
	world[0]._42 = 0.4f;   world[1]._42 = 0.4f;  world[2]._42 = 0.4f;
	world[0]._43 = 0;      world[1]._43 = 0.866f;  world[2]._43 = -0.866f;

	D3DXMatrixMultiply(&world[0], &world[0], &tmp);
	D3DXMatrixMultiply(&world[2], &world[2], &tmp);
	D3DXMatrixMultiply(&world[1], &world[1], &tmp);

	D3DXMatrixInverse(&inv[0], NULL, &world[0]);
	D3DXMatrixInverse(&inv[1], NULL, &world[1]);
	D3DXMatrixInverse(&inv[2], NULL, &world[2]);
	
	// specular effect uniforms
	D3DXMatrixMultiply(&vp, &view, &proj);
	D3DXMatrixInverse(&invview, NULL, &view);

	specular->SetMatrix("matViewProj", &vp);
	specular->SetVector("eyePos", (D3DXVECTOR4*)invview.m[3]);
	specular->SetVector("lightPos", &lightpos);

	// distance effect uniforms
	D3DXMatrixPerspectiveFovLH(&lightproj, D3DX_PI / 2, 1, 0.1f, 20.0f);
	distance->SetVector("lightPos", &lightpos);

	// shadow effect uniforms
	D3DXMatrixScaling(&world2[0], 5, 0.1f, 5);
	D3DXMatrixScaling(&world2[1], 5, 1.5f, 0.1f);
	D3DXMatrixScaling(&world2[2], 0.1f, 1.5f, 5);

	world2[1]._42 = 0.75f;
	world2[1]._43 = 2.5f;

	world2[2]._42 = 0.75f;
	world2[2]._41 = 2.5f;

	variance->SetMatrix("matViewProj", &vp);
	variance->SetMatrix("lightView", &lightview[0]);
	variance->SetVector("lightPos", &lightpos);

	if( SUCCEEDED(device->BeginScene()) )
	{
		// STEP 1: render scene six times into cubemap
		viewport.Width = viewport.Height = SHADOWMAP_SIZE;

		device->GetRenderTarget(0, &oldsurface);
		device->GetDepthStencilSurface(&olddepthstencil);

		device->SetRenderTarget(0, shadowsurf);
		device->SetDepthStencilSurface(depthstencil);
		device->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);

		viewport.X = 0;
		viewport.Y = SHADOWMAP_SIZE;

		for( int i = 0; i < 6; ++i )
		{
			switch( i )
			{
			case 1:
				viewport.X = SHADOWMAP_SIZE * 2;
				break;

			case 2:
				viewport.X = SHADOWMAP_SIZE;
				break;

			case 3:
				viewport.X = SHADOWMAP_SIZE * 3;
				break;

			case 4:
				viewport.X = SHADOWMAP_SIZE;
				viewport.Y = 0;
				break;

			case 5:
				viewport.Y = SHADOWMAP_SIZE * 2;
				break;

			default:
				break;
			}

			device->SetViewport(&viewport);
			device->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);

			D3DXMatrixMultiply(&lightvp, &lightview[i], &lightproj);

			distance->SetMatrix("lightViewProj", &lightvp);
			distance->SetMatrix("matWorld", &world[0]);

			distance->Begin(NULL, 0);
			distance->BeginPass(0);
			{
				shadowcaster->DrawSubset(0);

				distance->SetMatrix("matWorld", &world[1]);
				distance->CommitChanges();

				shadowcaster->DrawSubset(0);

				distance->SetMatrix("matWorld", &world[2]);
				distance->CommitChanges();

				shadowcaster->DrawSubset(0);

				// receivers
				distance->SetMatrix("matWorld", &world2[0]);
				distance->CommitChanges();

				shadowreceiver->DrawSubset(0);

				distance->SetMatrix("matWorld", &world2[1]);
				distance->CommitChanges();

				shadowreceiver->DrawSubset(0);

				distance->SetMatrix("matWorld", &world2[2]);
				distance->CommitChanges();

				shadowreceiver->DrawSubset(0);
			}
			distance->EndPass();
			distance->End();
		}

		// STEP 2: blur shadow map
		viewport.Width = SHADOWMAP_SIZE * 4;
		viewport.Height = SHADOWMAP_SIZE * 4;
		viewport.X = viewport.Y = 0;
	
		device->SetViewport(&viewport);
		device->SetVertexDeclaration(vertexdecl);

		BlurShadowmap("blurx", shadowmap, blurshadowmap);
		BlurShadowmap("blury", blurshadowmap, shadowmap);
		
		// STEP 3: render scene
		viewport.Width = screenwidth;
		viewport.Height = screenheight;
		viewport.X = viewport.Y = 0;

		device->SetRenderTarget(0, oldsurface);
		device->SetDepthStencilSurface(olddepthstencil);
		device->SetViewport(&viewport);
		device->Clear(0, NULL, flags, 0xff6694ed, 1.0f, 0);

		oldsurface->Release();
		olddepthstencil->Release();

		device->SetTexture(0, texture2);
		device->SetTexture(1, shadowmap);
		device->SetTexture(2, virtualcube);

		// receiver
		D3DXVECTOR4 uv(2, 2, 0, 1);

		variance->SetMatrix("matWorld", &world2[0]);
		variance->SetVector("uv", &uv);

		variance->Begin(NULL, 0);
		variance->BeginPass(0);
		{
			shadowreceiver->DrawSubset(0);

			uv.y = 1;

			variance->SetMatrix("matWorld", &world2[1]);
			variance->SetVector("uv", &uv);
			variance->CommitChanges();

			shadowreceiver->DrawSubset(0);

			variance->SetMatrix("matWorld", &world2[2]);
			variance->CommitChanges();

			shadowreceiver->DrawSubset(0);
		}
		variance->EndPass();
		variance->End();

		// caster
		device->SetTexture(0, texture1);

		specular->SetMatrix("matWorld", &world[0]);
		specular->SetMatrix("matWorldInv", &inv[0]);

		specular->Begin(NULL, 0);
		specular->BeginPass(0);
		{
			shadowcaster->DrawSubset(0);

			specular->SetMatrix("matWorld", &world[1]);
			specular->SetMatrix("matWorldInv", &inv[1]);
			specular->CommitChanges();

			shadowcaster->DrawSubset(0);

			specular->SetMatrix("matWorld", &world[2]);
			specular->SetMatrix("matWorldInv", &inv[2]);
			specular->CommitChanges();

			shadowcaster->DrawSubset(0);
		}
		specular->EndPass();
		specular->End();

		//DebugVisualize(shadowmap, 512, 512);

		device->SetTexture(1, NULL);
		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
void BlurShadowmap(const char* technique, LPDIRECT3DTEXTURE9 src, LPDIRECT3DTEXTURE9 dest)
{
	LPDIRECT3DSURFACE9 surface = NULL;
	dest->GetSurfaceLevel(0, &surface);

	device->SetRenderTarget(0, surface);
	device->SetTexture(0, src);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);

	blur->SetTechnique(technique);
	blur->SetVector("texelsize", &texelsize);
	blur->Begin(NULL, 0);
	blur->BeginPass(0);
	{
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices,
			sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
	}
	blur->EndPass();
	blur->End();

	surface->Release();
}
//*************************************************************************************************************
