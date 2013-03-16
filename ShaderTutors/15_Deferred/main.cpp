//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>

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

// external functions
HRESULT		DXCreateEffect(const char* file, LPD3DXEFFECT* out);
HRESULT		DXGenTangentFrame(LPD3DXMESH* mesh);
void		DXRenderText(const std::string& str, LPDIRECT3DTEXTURE9 tex, DWORD width, DWORD height);

// tutorial classes
class PointLight
{
public:
	LPDIRECT3DCUBETEXTURE9	ShadowMap;
	LPDIRECT3DCUBETEXTURE9	Blur;

	D3DXVECTOR4		Color;
	D3DXVECTOR3		Position;
	float			Radius;

	void GetScissorRect(RECT& out, const D3DXMATRIX& view, const D3DXMATRIX& proj) const;
};

class DirectionalLight
{
public:
	LPDIRECT3DTEXTURE9	ShadowMap;
	LPDIRECT3DTEXTURE9	Blur;

	D3DXMATRIX		ViewProj;
	D3DXVECTOR4		Color;
	D3DXVECTOR4		Direction;
};

// tutorial variables
LPD3DXEFFECT					rendergbuffer	= NULL;
LPD3DXEFFECT					deferred		= NULL;
LPD3DXEFFECT					distance		= NULL;
LPDIRECT3DVERTEXDECLARATION9	quaddecl		= NULL;
LPDIRECT3DVERTEXDECLARATION9	blurdecl		= NULL;

LPD3DXMESH						mesh1			= NULL;
LPD3DXMESH						mesh2			= NULL;
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
D3DXMATRIX		world[4];
D3DXMATRIX		shadowproj;
int				currentlight = 0;

PointLight pointlights[] =
{
	{ 0, 0, D3DXCOLOR(1, 0, 0, 1), D3DXVECTOR3(1.5f, 0.5f, 0), 5 },
	{ 0, 0, D3DXCOLOR(0, 1, 0, 1), D3DXVECTOR3(-0.7f, 0.5f, 1.2f), 5 },
	{ 0, 0, D3DXCOLOR(0, 0, 1, 1), D3DXVECTOR3(0, 0.5f, 0), 5 },
};

DirectionalLight directionallights[] =
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

	MYVALID(D3DXLoadMeshFromX("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh2));
	MYVALID(D3DXLoadMeshFromX("../media/meshes/skullocc3.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2.jpg", &texture2));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/static_sky.jpg", &texture3));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2_normal.tga", &texture4));

	MYVALID(device->CreateTexture(512, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &albedo, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &scene, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &normaldepth, NULL));

	MYVALID(DXCreateEffect("../media/shaders/gbuffer.fx", &rendergbuffer));
	MYVALID(DXCreateEffect("../media/shaders/deferred.fx", &deferred));
	MYVALID(DXCreateEffect("../media/shaders/distance.fx", &distance));

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

	MYVALID(DXGenTangentFrame(&mesh2));
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
	SAFE_RELEASE(mesh2);
	SAFE_RELEASE(mesh1);
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
		// skull 1
		device->SetTexture(0, texture1);
		mesh1->DrawSubset(0);

		// skull 2
		D3DXMatrixInverse(&inv, NULL, &world[1]);

		effect->SetMatrix("matWorld", &world[1]);
		effect->SetMatrix("matWorldInv", &inv);
		effect->CommitChanges();

		mesh1->DrawSubset(0);

		// skull 3
		D3DXMatrixInverse(&inv, NULL, &world[0]);

		effect->SetMatrix("matWorld", &world[2]);
		effect->SetMatrix("matWorldInv", &inv);
		effect->CommitChanges();

		mesh1->DrawSubset(0);
	}
	effect->EndPass();
	effect->End();

	if( tech )
		effect->SetTechnique(tech);

	effect->Begin(NULL, 0);
	effect->BeginPass(0);
	{
		// table
		D3DXMatrixInverse(&inv, NULL, &world[3]);

		effect->SetMatrix("matWorldInv", &inv);
		effect->SetMatrix("matWorld", &world[3]);
		effect->CommitChanges();

		device->SetTexture(0, texture2);
		device->SetTexture(1, texture4);

		mesh2->DrawSubset(0);
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

	// skull 1
	mesh1->DrawSubset(0);

	// skull 2
	effect->SetMatrix("matWorld", &world[1]);
	effect->CommitChanges();

	mesh1->DrawSubset(0);

	// skull 3
	effect->SetMatrix("matWorld", &world[2]);
	effect->CommitChanges();

	mesh1->DrawSubset(0);

	// table
	distance->SetMatrix("matWorld", &world[3]);
	distance->CommitChanges();

	mesh2->DrawSubset(0);
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
			DirectionalLight& lt = directionallights[i];

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
		const PointLight& lt = pointlights[i];

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
				DirectionalLight& lt = directionallights[i];

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
			const PointLight& lt = pointlights[i];

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
				const DirectionalLight& lt = directionallights[i];

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
			const PointLight& lt = pointlights[i];

			if( currentlight > 0 && ((i + 1) != currentlight) )
				continue;

			lightpos = D3DXVECTOR4(lt.Position, lt.Radius);

			deferred->SetVector("lightColor", &lt.Color);
			deferred->SetVector("lightPos", &lightpos);
			deferred->CommitChanges();

			lt.GetScissorRect(scissorbox, view, proj);

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
	D3DXVECTOR3 look(0, 0.5f, 0);
	D3DXVECTOR3 up(0, 1, 0);

	eye = D3DXVECTOR3(-3, 3, -3);

	D3DXMatrixRotationY(&world[0], -0.03f * time);
	D3DXVec3TransformCoord(&eye, &eye, &world[0]);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)screenwidth / (float)screenheight, 0.1f, 100);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixMultiply(&viewproj, &view, &proj);

	// lights
	D3DXVECTOR3 dir(-0.2f, 0.65f, 1);

	D3DXVec3TransformCoord(&dir, &dir, &world[0]);

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
	D3DXMatrixScaling(&world[0], 0.15f, 0.15f, 0.15f);
	D3DXMatrixScaling(&world[1], 0.15f, 0.15f, 0.15f);
	D3DXMatrixScaling(&world[2], 0.15f, 0.15f, 0.15f);

	world[0]._41 = -1.5;
	world[0]._43 = 1.5;

	world[1]._41 = 1.5;
	world[1]._43 = 1.5;

	world[2]._41 = 0;
	world[2]._43 = -1;

	// render
	if( SUCCEEDED(device->BeginScene()) )
	{
		// shadow pass
		D3DXMatrixScaling(&world[3], 20, 0.1f, 20); // cheat

		DrawShadowsMaps();

		// draw scene & lights
		D3DXMatrixScaling(&world[3], 5, 0.1f, 5);

		DrawDeferred();

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
void PointLight::GetScissorRect(RECT& out, const D3DXMATRIX& view, const D3DXMATRIX& proj) const
{
	D3DXVECTOR4 Q;
	D3DXVECTOR3 L, P1, P2;
	float u, v;

	out.left	= 0;
	out.right	= screenwidth;
	out.top		= 0;
	out.bottom	= screenheight;

	D3DXVec3TransformCoord(&L, &Position, &view);

	float d = 4 * (Radius * Radius * L.x * L.x - (L.x * L.x + L.z * L.z) * (Radius * Radius - L.z * L.z));

	if( d >= 0 && L.z >= 0.0f )
	{
		float a1 = (Radius * L.x + sqrtf(d * 0.25f)) / (L.x * L.x + L.z * L.z);
		float a2 = (Radius * L.x - sqrtf(d * 0.25f)) / (L.x * L.x + L.z * L.z);
		float c1 = (Radius - a1 * L.x) / L.z;
		float c2 = (Radius - a2 * L.x) / L.z;

		P1.z = (L.x * L.x + L.z * L.z - Radius * Radius) / (L.z - (c1 / a1) * L.x);
		P1.x = (-P1.z * c1) / a1;
		P1.y = 0;

		P2.z = (L.x * L.x + L.z * L.z - Radius * Radius) / (L.z - (c2 / a2) * L.x);
		P2.x = (-P2.z * c2) / a2;
		P2.y = 0;

		if( P1.z > 0 && P2.z > 0 )
		{
			D3DXVec3Transform(&Q, &P1, &proj);

			Q /= Q.w;
			u = (Q.x * 0.5f + 0.5f) * screenwidth;

			D3DXVec3Transform(&Q, &P2, &proj);

			Q /= Q.w;
			v = (Q.x * 0.5f + 0.5f) * screenwidth;

			if( P2.x < L.x )
			{
				out.left = (LONG)max(v, 0);
				out.right = (LONG)min(u, screenwidth);
			}
			else
			{
				out.left = (LONG)max(u, 0);
				out.right = (LONG)min(v, screenwidth);
			}
		}
	}

	d = 4 * (Radius * Radius * L.y * L.y - (L.y * L.y + L.z * L.z) * (Radius * Radius - L.z * L.z));

	if( d >= 0 && L.z >= 0.0f )
	{
		float b1 = (Radius * L.y + sqrtf(d * 0.25f)) / (L.y * L.y + L.z * L.z);
		float b2 = (Radius * L.y - sqrtf(d * 0.25f)) / (L.y * L.y + L.z * L.z);
		float c1 = (Radius - b1 * L.y) / L.z;
		float c2 = (Radius - b2 * L.y) / L.z;

		P1.z = (L.y * L.y + L.z * L.z - Radius * Radius) / (L.z - (c1 / b1) * L.y);
		P1.y = (-P1.z * c1) / b1;
		P1.x = 0;

		P2.z = (L.y * L.y + L.z * L.z - Radius * Radius) / (L.z - (c2 / b2) * L.y);
		P2.y = (-P2.z * c2) / b2;
		P2.x = 0;

		if( P1.z > 0 && P2.z > 0 )
		{
			D3DXVec3Transform(&Q, &P1, &proj);

			Q /= Q.w;
			u = (Q.y * -0.5f + 0.5f) * screenheight;

			D3DXVec3Transform(&Q, &P2, &proj);

			Q /= Q.w;
			v = (Q.y * -0.5f + 0.5f) * screenheight;

			if( P2.y < L.y )
			{
				out.top = (LONG)max(u, 0);
				out.bottom = (LONG)min(v, screenheight);
			}
			else
			{
				out.top = (LONG)max(v, 0);
				out.bottom = (LONG)min(u, screenheight);
			}
		}
	}
}
//*************************************************************************************************************
