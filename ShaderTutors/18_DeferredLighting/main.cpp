//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>

#include "../common/dxext.h"

// - sky
// - szetvalasztani az atlatszot
// - deferredlighting

// helper macros
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

#define SHADOWMAP_SIZE		512
#define CUBESHADOWMAP_SIZE	256

// external variables
extern long screenwidth;
extern long screenheight;

extern LPDIRECT3DDEVICE9 device;

// tutorial variables
LPD3DXEFFECT					rendergbuffer	= NULL;
LPD3DXEFFECT					deferred		= NULL;
LPD3DXEFFECT					distance		= NULL;
LPDIRECT3DVERTEXDECLARATION9	quaddecl		= NULL;
LPDIRECT3DVERTEXDECLARATION9	blurdecl		= NULL;

DXObject*						object1			= NULL;
DXObject*						object2			= NULL;
LPDIRECT3DTEXTURE9				texture1		= NULL;
LPDIRECT3DTEXTURE9				texture2		= NULL;
LPDIRECT3DTEXTURE9				texture3		= NULL;
LPDIRECT3DTEXTURE9				texture4		= NULL;

LPDIRECT3DTEXTURE9				albedo			= NULL;
LPDIRECT3DTEXTURE9				normaldepth		= NULL;
LPDIRECT3DTEXTURE9				scene			= NULL;
LPDIRECT3DTEXTURE9				text			= NULL;

LPDIRECT3DSURFACE9				albedosurf		= NULL;
LPDIRECT3DSURFACE9				normaldepthsurf	= NULL;
LPDIRECT3DSURFACE9				scenesurf		= NULL;

D3DXVECTOR3		eye(-3, 3, -3);
D3DXVECTOR4		scenesize(2.0f / (float)screenwidth, 2.0f / (float)screenheight, 0, 1);
D3DXMATRIX		view, proj, viewproj;
D3DXMATRIX		world[2];
D3DXMATRIX		shadowproj;
int				currentlight = 0;

DXPointLight pointlights[] =
{
	{ 0, 0, D3DXCOLOR(1, 0, 0, 1), D3DXVECTOR3(1.5f, 0.5f, 0), 5 },
	{ 0, 0, D3DXCOLOR(0, 1, 0, 1), D3DXVECTOR3(-0.7f, 0.5f, 1.2f), 5 },
	{ 0, 0, D3DXCOLOR(0, 0, 1, 1), D3DXVECTOR3(0, 0.5f, 0), 5 },
};

DXDirectionalLight directionallights[] =
{
	{ 0, 0, D3DXMATRIX(), D3DXCOLOR(0.6f, 0.6f, 1, 1), D3DXVECTOR4(0, 0, 0, 5) },
};

D3DXVECTOR3 cubefwd[6] =
{
	D3DXVECTOR3(1, 0, 0),
	D3DXVECTOR3(-1, 0, 0),
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, -1, 0),
	D3DXVECTOR3(0, 0, 1),
	D3DXVECTOR3(0, 0, -1),
};

D3DXVECTOR3 cubeup[6] =
{
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, 0, -1),
	D3DXVECTOR3(0, 0, 1),
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, 1, 0),
};

float quadvertices[36] =
{
						-0.5f,						-0.5f, 0, 1,	0, 0,
	(float)screenwidth - 0.5f,						-0.5f, 0, 1,	1, 0,
						-0.5f, (float)screenheight - 0.5f, 0, 1,	0, 1,

						-0.5f, (float)screenheight - 0.5f, 0, 1,	0, 1,
	(float)screenwidth - 0.5f,						-0.5f, 0, 1,	1, 0,
	(float)screenwidth - 0.5f, (float)screenheight - 0.5f, 0, 1,	1, 1
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

float blurvertices[36] =
{
	-0.5f,							-0.5f,							0, 1,	0, 0,
	(float)SHADOWMAP_SIZE - 0.5f,	-0.5f,							0, 1,	1, 0,
	-0.5f,							(float)SHADOWMAP_SIZE - 0.5f,	0, 1,	0, 1,
	
	-0.5f,							(float)SHADOWMAP_SIZE - 0.5f,	0, 1,	0, 1,
	(float)SHADOWMAP_SIZE - 0.5f,	-0.5f, 0, 1,	1, 0,
	(float)SHADOWMAP_SIZE - 0.5f,	(float)SHADOWMAP_SIZE - 0.5f,	0, 1,	1, 1
};

float cubevertices[24 * 5] =
{
	// X+
	1, -1, 1,	0, 1,
	1, 1, 1,	0, 0,
	1, 1, -1,	1, 0,
	1, -1, -1,	1, 1,

	// X-
	-1, -1, -1,	0, 1,
	-1, 1, -1,	0, 0,
	-1, 1, 1,	1, 0,
	-1, -1, 1,	1, 1,

	// Y+
	-1, 1, 1,	0, 1,
	-1, 1, -1,	0, 0,
	1, 1, -1,	1, 0,
	1, 1, 1,	1, 1,

	// Y-
	-1, -1, -1,	0, 1,
	-1, -1, 1,	0, 0,
	1, -1, 1,	1, 0,
	1, -1, -1,	1, 1,

	// Z+
	-1, -1, 1,	0, 1,
	-1, 1, 1,	0, 0,
	1, 1, 1,	1, 0,
	1, -1, 1,	1, 1,

	// Z-
	1, -1, -1,	0, 1,
	1, 1, -1,	0, 0,
	-1, 1, -1,	1, 0,
	-1, -1, -1,	1, 1,
};

unsigned short cubeindices[6] =
{
	0, 1, 2, 0, 2, 3
};

static const int NUM_POINT_LIGHTS = sizeof(pointlights) / sizeof(pointlights[0]);
static const int NUM_DIRECTIONAL_LIGHTS = sizeof(directionallights) / sizeof(directionallights[0]);
static const UINT vertexstride = 6 * sizeof(float);

// tutorial functions
void DrawScene(LPD3DXEFFECT effect);
void DrawSceneForShadow(LPD3DXEFFECT effect);
void DrawDeferred();
void DrawShadowsMaps();
void BlurShadowMaps();

HRESULT InitScene()
{
	HRESULT hr;
	D3DCAPS9 caps;

	device->GetDeviceCaps(&caps);

	if( caps.VertexShaderVersion < D3DVS_VERSION(3, 0) || caps.PixelShaderVersion < D3DPS_VERSION(3, 0) )
	{
		MYERROR("This demo requires Shader Model 3.0 capable video card");
		return E_FAIL;
	}

	if( caps.NumSimultaneousRTs < 3 )
	{
		MYERROR("This demo requires at least 3 simultaneous render targets");
		return E_FAIL;
	}

	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	D3DVERTEXELEMENT9 elem2[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	MYVALID(device->CreateVertexDeclaration(elem, &quaddecl));
	MYVALID(device->CreateVertexDeclaration(elem2, &blurdecl));

	object1 = new DXObject(device);
	object2 = new DXObject(device);

	if( !object1->Load("../media/meshes/bridge/bridge.qm") )
	{
		MYERROR("object1->Load(\"../media/meshes/bridge/bridge.qm\")");
		return E_FAIL;
	}

	if( !object2->Load("../media/meshes/reventon/reventon.qm") )
	{
		MYERROR("object1->Load(\"../media/meshes/reventon/reventon.qm\")");
		return E_FAIL;
	}

	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/static_sky.jpg", &texture3));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/meshes/bridge/bridge_normal.tga", &texture4));

	MYVALID(device->CreateTexture(512, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &albedo, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &scene, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &normaldepth, NULL));

	MYVALID(DXCreateEffect("../media/shaders/gbuffer.fx", device, &rendergbuffer));
	MYVALID(DXCreateEffect("../media/shaders/deferred.fx", device, &deferred));
	MYVALID(DXCreateEffect("../media/shaders/distance.fx", device, &distance));

	MYVALID(albedo->GetSurfaceLevel(0, &albedosurf));
	MYVALID(normaldepth->GetSurfaceLevel(0, &normaldepthsurf));
	MYVALID(scene->GetSurfaceLevel(0, &scenesurf));

	for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
	{
		MYVALID(device->CreateCubeTexture(CUBESHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &pointlights[i].ShadowMap, NULL));
		MYVALID(device->CreateCubeTexture(CUBESHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &pointlights[i].Blur, NULL));
	}

	for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
	{
		MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &directionallights[i].ShadowMap, NULL));
		MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &directionallights[i].Blur, NULL));
	}

	DXRenderText("Press buttons 0-3 to view lights", text, 512, 128);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
	{
		if( pointlights[i].ShadowMap )
			pointlights[i].ShadowMap->Release();

		if( pointlights[i].Blur )
			pointlights[i].Blur->Release();
	}

	for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
	{
		if( directionallights[i].ShadowMap )
			directionallights[i].ShadowMap->Release();

		if( directionallights[i].Blur )
			directionallights[i].Blur->Release();
	}

	delete object1;
	delete object2;

	SAFE_RELEASE(scenesurf);
	SAFE_RELEASE(normaldepthsurf);
	SAFE_RELEASE(albedosurf);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(texture2);
	SAFE_RELEASE(texture3);
	SAFE_RELEASE(texture4);
	SAFE_RELEASE(scene);
	SAFE_RELEASE(text);
	SAFE_RELEASE(normaldepth);
	SAFE_RELEASE(albedo);
	SAFE_RELEASE(quaddecl);
	SAFE_RELEASE(blurdecl);
	SAFE_RELEASE(rendergbuffer);
	SAFE_RELEASE(deferred);
	SAFE_RELEASE(distance);
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	if( wparam >= 0x30 && wparam <= 0x39 )
	{
		if( (wparam - 0x30) <= NUM_POINT_LIGHTS )
			currentlight = wparam - 0x30;
	}
}
//*************************************************************************************************************
void Update(float delta)
{
	// do nothing
}
//*************************************************************************************************************
void DrawScene(LPD3DXEFFECT effect)
{
	D3DXMATRIX inv;
	D3DXHANDLE oldtech = effect->GetCurrentTechnique();
	D3DXHANDLE tech = effect->GetTechniqueByName("gbuffer_tbn");

	D3DXMatrixInverse(&inv, NULL, &world[0]);

	effect->SetMatrix("matWorld", &world[0]);
	effect->SetMatrix("matWorldInv", &inv);
	effect->SetMatrix("matViewProj", &viewproj);

	effect->Begin(NULL, 0);
	effect->BeginPass(0);
	{
		object2->Draw(effect);
	}
	effect->EndPass();
	effect->End();

	if( tech )
		effect->SetTechnique(tech);

	effect->Begin(NULL, 0);
	effect->BeginPass(0);
	{
		// bridge
		D3DXMatrixInverse(&inv, NULL, &world[1]);

		effect->SetMatrix("matWorldInv", &inv);
		effect->SetMatrix("matWorld", &world[1]);
		effect->CommitChanges();

		device->SetTexture(1, texture4);

		object1->Draw();
	}
	effect->EndPass();
	effect->End();

	if( tech )
		effect->SetTechnique(oldtech);
}
//*************************************************************************************************************
void DrawSceneForShadow(LPD3DXEFFECT effect)
{
	effect->SetMatrix("matWorld", &world[0]);
	effect->CommitChanges();

	object2->Draw();

	// bridge
	distance->SetMatrix("matWorld", &world[1]);
	distance->CommitChanges();

	object1->DrawSubset(0);
}
//*************************************************************************************************************
void DrawShadowsMaps()
{
	LPDIRECT3DSURFACE9	oldsurface = NULL;
	LPDIRECT3DSURFACE9	surface = NULL;
	D3DXMATRIX			shadowview;
	D3DXMATRIX			shadowvp;
	D3DXVECTOR4			origin(0, 0, 0, 0);
	D3DXVECTOR3			up(0, 1, 0);

	device->GetRenderTarget(0, &oldsurface);

	// directional lights
	if( currentlight == 0 )
	{
		distance->SetTechnique("distance_directional");
		distance->Begin(NULL, 0);
		distance->BeginPass(0);

		D3DXMatrixOrthoLH(&shadowproj, 7, 7, 0.1f, 100);

		for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
		{
			DXDirectionalLight& lt = directionallights[i];

			D3DXMatrixLookAtLH(&shadowview, (D3DXVECTOR3*)&(origin + lt.Direction.w * lt.Direction), (D3DXVECTOR3*)&origin, &up);
			D3DXMatrixMultiply(&lt.ViewProj, &shadowview, &shadowproj);

			distance->SetVector("lightPos", &lt.Direction);
			distance->SetMatrix("matViewProj", &lt.ViewProj);

			lt.ShadowMap->GetSurfaceLevel(0, &surface);

			device->SetRenderTarget(0, surface);
			device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0);

			DrawSceneForShadow(distance);
			surface->Release();
		}

		distance->EndPass();
		distance->End();
	}

	// point lights
	distance->SetTechnique("distance_point");
	distance->Begin(NULL, 0);
	distance->BeginPass(0);

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
	{
		const DXPointLight& lt = pointlights[i];

		if( currentlight > 0 && ((i + 1) != currentlight) )
			continue;

		D3DXMatrixPerspectiveFovLH(&shadowproj, D3DX_PI / 2, 1, 0.1f, lt.Radius);
		distance->SetVector("lightPos", (D3DXVECTOR4*)&lt.Position);

		for( int j = 0; j < 6; ++j )
		{
			D3DXMatrixLookAtLH(&shadowview, &lt.Position, &(lt.Position + cubefwd[j]), &cubeup[j]);
			D3DXMatrixMultiply(&shadowvp, &shadowview, &shadowproj);

			distance->SetMatrix("matViewProj", &shadowvp);

			lt.ShadowMap->GetCubeMapSurface((D3DCUBEMAP_FACES)j, 0, &surface);

			device->SetRenderTarget(0, surface);
			device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0);

			DrawSceneForShadow(distance);
			surface->Release();
		}
	}

	distance->EndPass();
	distance->End();

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	BlurShadowMaps();

	device->SetRenderState(D3DRS_ZENABLE, TRUE);
	device->SetRenderTarget(0, oldsurface);

	oldsurface->Release();
}
//*************************************************************************************************************
void BlurShadowMaps()
{
	LPDIRECT3DSURFACE9	surface = NULL;
	D3DXMATRIX			shadowview;

	device->SetVertexDeclaration(quaddecl);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);

	// blur directional shadows
	if( currentlight == 0 )
	{
		D3DXVECTOR4 texelsize(1.0f / SHADOWMAP_SIZE, 0, 0, 0);

		distance->SetTechnique("blur5x5");
		distance->Begin(NULL, 0);
		distance->BeginPass(0);
		{
			for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
			{
				DXDirectionalLight& lt = directionallights[i];

				// x
				lt.Blur->GetSurfaceLevel(0, &surface);

				distance->SetVector("texelSize", &texelsize);
				distance->CommitChanges();

				device->SetRenderTarget(0, surface);
				device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

				device->SetTexture(0, lt.ShadowMap);
				device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, blurvertices, vertexstride);

				surface->Release();
				std::swap(texelsize.x, texelsize.y);

				// y
				lt.ShadowMap->GetSurfaceLevel(0, &surface);

				distance->SetVector("texelSize", &texelsize);
				distance->CommitChanges();

				device->SetRenderTarget(0, surface);
				device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

				device->SetTexture(0, lt.Blur);
				device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, blurvertices, vertexstride);

				surface->Release();
				std::swap(texelsize.x, texelsize.y);
			}
		}
		distance->EndPass();
		distance->End();
	}

	// blur point shadows
	D3DXVECTOR4 texelsizesx[6] =
	{
		D3DXVECTOR4(0, 0, 2.0f / CUBESHADOWMAP_SIZE, 0),
		D3DXVECTOR4(0, 0, 2.0f / CUBESHADOWMAP_SIZE, 0),

		D3DXVECTOR4(2.0f / CUBESHADOWMAP_SIZE, 0, 0, 0),
		D3DXVECTOR4(2.0f / CUBESHADOWMAP_SIZE, 0, 0, 0),

		D3DXVECTOR4(2.0f / CUBESHADOWMAP_SIZE, 0, 0, 0),
		D3DXVECTOR4(2.0f / CUBESHADOWMAP_SIZE, 0, 0, 0),
	};

	D3DXVECTOR4 texelsizesy[6] =
	{
		D3DXVECTOR4(0, 2.0f / CUBESHADOWMAP_SIZE, 0, 0),
		D3DXVECTOR4(0, 2.0f / CUBESHADOWMAP_SIZE, 0, 0),

		D3DXVECTOR4(0, 0, 2.0f / CUBESHADOWMAP_SIZE, 0),
		D3DXVECTOR4(0, 0, 2.0f / CUBESHADOWMAP_SIZE, 0),

		D3DXVECTOR4(0, 2.0f / CUBESHADOWMAP_SIZE, 0, 0),
		D3DXVECTOR4(0, 2.0f / CUBESHADOWMAP_SIZE, 0, 0),
	};

	UINT stride = 5 * sizeof(float);
	char* vert = (char*)&cubevertices[0];

	device->SetVertexDeclaration(blurdecl);

	distance->SetTechnique("blurcube5x5");
	distance->Begin(NULL, 0);
	distance->BeginPass(0);
	{
		for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
		{
			const DXPointLight& lt = pointlights[i];

			if( currentlight > 0 && ((i + 1) != currentlight) )
				continue;

			for( int j = 0; j < 6; ++j )
			{
				D3DXMatrixLookAtLH(&shadowview, &D3DXVECTOR3(0, 0, 0), &cubefwd[j], &cubeup[j]);

				// x
				distance->SetMatrix("matViewProj", &shadowview);
				distance->SetVector("texelSize", &texelsizesx[j]);
				distance->CommitChanges();

				lt.Blur->GetCubeMapSurface((D3DCUBEMAP_FACES)j, 0, &surface);

				device->SetRenderTarget(0, surface);
				device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

				device->SetTexture(0, lt.ShadowMap);
				device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, cubeindices, D3DFMT_INDEX16, vert + j * 4 * stride, stride);

				surface->Release();

				// y
				distance->SetVector("texelSize", &texelsizesy[j]);
				distance->CommitChanges();

				lt.ShadowMap->GetCubeMapSurface((D3DCUBEMAP_FACES)j, 0, &surface);

				device->SetRenderTarget(0, surface);
				device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

				device->SetTexture(0, lt.Blur);
				device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, cubeindices, D3DFMT_INDEX16,  vert + j * 4 * stride, stride);

				surface->Release();
			}
		}
	}
	distance->EndPass();
	distance->End();
}
//*************************************************************************************************************
void DrawDeferred()
{
	LPDIRECT3DSURFACE9	oldsurface = NULL;
	D3DXMATRIX			inv;
	D3DXVECTOR4			lightpos;
	RECT				scissorbox;

	// geometry pass
	device->GetRenderTarget(0, &oldsurface);
	device->SetRenderTarget(0, albedosurf);
	device->SetRenderTarget(1, normaldepthsurf);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);

	DrawScene(rendergbuffer);

	// deferred pass
	D3DXMatrixInverse(&inv, NULL, &viewproj);

	deferred->SetVector("eyePos", (D3DXVECTOR4*)&eye);
	deferred->SetMatrix("matViewProjInv", &inv);

	device->SetRenderTarget(0, scenesurf);
	device->SetRenderTarget(1, NULL);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);

	device->SetTexture(0, albedo);
	device->SetTexture(1, normaldepth);

	device->SetVertexDeclaration(quaddecl);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	// directional lights
	if( currentlight == 0 )
	{
		deferred->SetTechnique("deferred_directional");

		deferred->Begin(NULL, 0);
		deferred->BeginPass(0);
		{
			for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
			{
				const DXDirectionalLight& lt = directionallights[i];

				deferred->SetVector("lightColor", &lt.Color);
				deferred->SetVector("lightPos", &lt.Direction);
				deferred->SetMatrix("lightViewProj", &lt.ViewProj);
				deferred->CommitChanges();

				device->SetTexture(2, lt.ShadowMap);
				device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, vertexstride);
			}
		}
		deferred->EndPass();
		deferred->End();
	}

	// point lights
	deferred->SetTechnique("deferred_point");

	device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

	deferred->Begin(NULL, 0);
	deferred->BeginPass(0);
	{
		for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
		{
			const DXPointLight& lt = pointlights[i];

			if( currentlight > 0 && ((i + 1) != currentlight) )
				continue;

			lightpos = D3DXVECTOR4(lt.Position, lt.Radius);

			deferred->SetVector("lightColor", &lt.Color);
			deferred->SetVector("lightPos", &lightpos);
			deferred->CommitChanges();

			lt.GetScissorRect(scissorbox, view, proj, screenwidth, screenheight);

			device->SetTexture(2, lt.ShadowMap);
			device->SetScissorRect(&scissorbox);
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, vertexstride);
		}
	}
	deferred->EndPass();
	deferred->End();

	device->SetTexture(1, NULL);
	device->SetTexture(2, NULL);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	// combine with sky & gamma correct
	device->SetRenderTarget(0, oldsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

	oldsurface->Release();

	device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
	device->SetTexture(0, texture3);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, vertexstride);

	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);

	device->SetTexture(0, scene);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, vertexstride);

	// render text
	device->SetTexture(0, text);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, textvertices, vertexstride);

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_ZENABLE, TRUE);
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	// camera
	D3DXVECTOR3 look(-1, 0.3f, -0.4f);
	D3DXVECTOR3 up(0, 1, 0);

	eye = D3DXVECTOR3(-3, 2, -2.4f);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)screenwidth / (float)screenheight, 0.1f, 100);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixMultiply(&viewproj, &view, &proj);

	// lights
	D3DXVECTOR3 dir(-0.2f, 0.65f, 1);

	directionallights[0].Direction.x = dir.x;
	directionallights[0].Direction.y = dir.y;
	directionallights[0].Direction.z = dir.z;

	pointlights[0].Position.x = cosf(time * 0.5f) * 2;
	pointlights[0].Position.z = sinf(time * 0.5f) * cosf(time * 0.5f) * 2;

	pointlights[1].Position.x = cosf(1.5f * time) * 2;
	pointlights[1].Position.z = sinf(1 * time) * 2;

	pointlights[2].Position.x = cosf(0.75f * time) * 1.5f;
	pointlights[2].Position.z = sinf(1.5f * time) * 1.5f;

	time += elapsedtime;

	// world matrices
	D3DXMatrixScaling(&world[0], 0.5f, 0.5f, 0.5f);
	D3DXMatrixScaling(&world[1], 1, 1, 1);

	world[0]._41 = 0;
	world[0]._42 = -0.25f;
	world[0]._43 = 0;

	world[1]._42 = -1;

	// render
	if( SUCCEEDED(device->BeginScene()) )
	{
		// shadow pass
		DrawShadowsMaps();

		// draw scene & lights
		DrawDeferred();

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
