//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>

#include "../common/dxext.h"

// - point shadow blur
// - depth envelope: B-be max, ha b < d akkor shadow

// helper macros
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// NOTE: if you want to use bigger than the screen resolution,
// create a depth-stencil surface for shadows
#define SHADOWMAP_SIZE		512
#define CUBESHADOWMAP_SIZE	256
#define CUBEMAP_SIZE		256

// external variables
extern long screenwidth;
extern long screenheight;

extern LPDIRECT3DDEVICE9 device;

// tutorial variables
LPD3DXMESH						skymesh			= NULL;
LPD3DXEFFECT					rendergbuffer	= NULL;
LPD3DXEFFECT					deferred		= NULL;
LPD3DXEFFECT					distance		= NULL;
LPD3DXEFFECT					skyeffect		= NULL;
LPD3DXEFFECT					finalpass		= NULL;
LPD3DXEFFECT					forward			= NULL;
LPDIRECT3DVERTEXDECLARATION9	quaddecl		= NULL;

DXObject*						object1			= NULL;
DXObject*						object2			= NULL;
DXObject*						object3			= NULL;
LPDIRECT3DTEXTURE9				texture3		= NULL;
LPDIRECT3DTEXTURE9				texture4		= NULL;
LPDIRECT3DCUBETEXTURE9			skytex			= NULL;
LPDIRECT3DCUBETEXTURE9			envcube			= NULL;
LPDIRECT3DTEXTURE9				text			= NULL;

LPDIRECT3DTEXTURE9				normalpower		= NULL;
LPDIRECT3DTEXTURE9				depth			= NULL;
LPDIRECT3DTEXTURE9				irradiance1		= NULL;
LPDIRECT3DTEXTURE9				irradiance2		= NULL;

LPDIRECT3DSURFACE9				normalsurf		= NULL;
LPDIRECT3DSURFACE9				depthsurf		= NULL;
LPDIRECT3DSURFACE9				irradsurf1		= NULL;
LPDIRECT3DSURFACE9				irradsurf2		= NULL;

D3DXMATRIX		world[3];
int				currentlight = 0;
bool			drawcar = true;
bool			drawgate = true;

struct ViewParams
{
	D3DXMATRIX		view;
	D3DXMATRIX		proj;
	D3DXVECTOR3		eye;
	D3DVIEWPORT9	viewport;
};

DXPointLight pointlights[] =
{
	DXPointLight(D3DXCOLOR(1, 0, 0, 1), D3DXVECTOR3(1.5f, 0.5f, 0), 5),
	DXPointLight(D3DXCOLOR(0, 1, 0, 1), D3DXVECTOR3(-0.7f, 0.5f, 1.2f), 5),
	DXPointLight(D3DXCOLOR(0, 0, 1, 1), D3DXVECTOR3(0, 0.5f, 0), 5)
};

DXDirectionalLight directionallights[] =
{
	DXDirectionalLight(D3DXCOLOR(0.45f, 0.45f, 0.45f, 1), D3DXVECTOR4(0.1f, 1, 0.7f, 5)),
	DXDirectionalLight(D3DXCOLOR(0.05f, 0.05f, 0.05f, 1), D3DXVECTOR4(0.5f, 0.3f, -1, 5))
};

DXSpotLight spotlights[] =
{
	DXSpotLight(D3DXCOLOR(1, 1, 1, 1), D3DXVECTOR3(-1, 0.1f, 0.3f), D3DXVECTOR3(-1, 0, 0), D3DX_PI / 4, D3DX_PI / 6, 10),
	DXSpotLight(D3DXCOLOR(1, 1, 1, 1), D3DXVECTOR3(-1, 0.1f, -0.3f), D3DXVECTOR3(-1, 0, 0), D3DX_PI / 4, D3DX_PI / 6, 10),
};

static const int NUM_POINT_LIGHTS = sizeof(pointlights) / sizeof(pointlights[0]);
static const int NUM_DIRECTIONAL_LIGHTS = sizeof(directionallights) / sizeof(directionallights[0]);
static const int NUM_SPOTLIGHTS = sizeof(spotlights) / sizeof(spotlights[0]);

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

// tutorial functions
void DrawScene(LPD3DXEFFECT effect, const D3DXMATRIX& viewproj, const D3DXVECTOR3& eye);
void DrawSceneForShadow(LPD3DXEFFECT effect);
void DrawDeferred(LPDIRECT3DSURFACE9 target, const ViewParams& params);
void DrawTransparent(const ViewParams& params);
void DrawEnvironment();
void DrawShadowMaps();
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

	MYVALID(device->CreateVertexDeclaration(elem, &quaddecl));

	object1 = new DXObject(device);
	object2 = new DXObject(device);
	object3 = new DXObject(device);

	if( !object1->Load("../media/meshes/bridge/bridge.qm") )
	{
		MYERROR("object1->Load(\"../media/meshes/bridge/bridge.qm\")");
		return E_FAIL;
	}

	if( !object2->Load("../media/meshes/reventon/reventon.qm") )
	{
		MYERROR("object2->Load(\"../media/meshes/reventon/reventon.qm\")");
		return E_FAIL;
	}

	if( !object3->Load("../media/meshes/gate/gate.qm") )
	{
		MYERROR("object3->Load(\"../media/meshes/gate/gate.qm\")");
		return E_FAIL;
	}

	MYVALID(D3DXLoadMeshFromXA("../media/meshes/sky.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skymesh));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/meshes/bridge/bridge_normal.tga", &texture4));
	MYVALID(D3DXCreateCubeTextureFromFileA(device, "../media/textures/sky4.dds", &skytex));

	MYVALID(device->CreateTexture(512, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));

	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &normalpower, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &depth, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &irradiance1, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &irradiance2, NULL));
	MYVALID(device->CreateCubeTexture(CUBEMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &envcube, NULL));

	MYVALID(DXCreateEffect("../media/shaders/gbuffer.fx", device, &rendergbuffer));
	MYVALID(DXCreateEffect("../media/shaders/deferred.fx", device, &deferred));
	MYVALID(DXCreateEffect("../media/shaders/distance.fx", device, &distance));
	MYVALID(DXCreateEffect("../media/shaders/sky.fx", device, &skyeffect));
	MYVALID(DXCreateEffect("../media/shaders/deferredlighting.fx", device, &finalpass));
	MYVALID(DXCreateEffect("../media/shaders/forward.fx", device, &forward));

	MYVALID(depth->GetSurfaceLevel(0, &depthsurf));
	MYVALID(normalpower->GetSurfaceLevel(0, &normalsurf));
	MYVALID(irradiance1->GetSurfaceLevel(0, &irradsurf1));
	MYVALID(irradiance2->GetSurfaceLevel(0, &irradsurf2));

	for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
		pointlights[i].CreateShadowMap(device, DXLight::Dynamic, CUBESHADOWMAP_SIZE);

	directionallights[0].CreateShadowMap(device, DXLight::Dynamic, SHADOWMAP_SIZE);

	for( int i = 1; i < NUM_DIRECTIONAL_LIGHTS; ++i )
		directionallights[i].CreateShadowMap(device, DXLight::Static, SHADOWMAP_SIZE);

	rendergbuffer->SetTechnique("gbuffer_dlighting");
	DXRenderText("Press buttons 0-3 to view lights", text, 512, 128);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	// don't do this, ever!
	for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
		pointlights[i].~DXPointLight();

	for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
		directionallights[i].~DXDirectionalLight();

	delete object1;
	delete object2;
	delete object3;

	SAFE_RELEASE(skymesh);
	SAFE_RELEASE(skytex);
	SAFE_RELEASE(skyeffect);
	SAFE_RELEASE(irradsurf1);
	SAFE_RELEASE(irradsurf2);
	SAFE_RELEASE(normalsurf);
	SAFE_RELEASE(depthsurf);
	SAFE_RELEASE(texture3);
	SAFE_RELEASE(texture4);
	SAFE_RELEASE(irradiance1);
	SAFE_RELEASE(irradiance2);
	SAFE_RELEASE(envcube);
	SAFE_RELEASE(text);
	SAFE_RELEASE(normalpower);
	SAFE_RELEASE(depth);
	SAFE_RELEASE(quaddecl);
	SAFE_RELEASE(rendergbuffer);
	SAFE_RELEASE(deferred);
	SAFE_RELEASE(distance);
	SAFE_RELEASE(finalpass);
	SAFE_RELEASE(forward);
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
void DrawScene(LPD3DXEFFECT effect, const D3DXMATRIX& viewproj, const D3DXVECTOR3& eye)
{
	D3DXMATRIX		inv;
	D3DXHANDLE		oldtech			= effect->GetCurrentTechnique();
	D3DXVECTOR4		params(1, 1, 1, 0);
	unsigned int	flags			= DXObject::Opaque;
	bool			drawforgbuffer	= (effect == rendergbuffer);
	bool			drawforfinal	= (effect == finalpass);

	if( drawforfinal )
		flags |= DXObject::Material;

	effect->SetMatrix("matViewProj", &viewproj);
	effect->SetVector("params", &params);

	// gate
	D3DXMatrixInverse(&inv, NULL, &world[2]);

	effect->SetMatrix("matWorld", &world[2]);
	effect->SetMatrix("matWorldInv", &inv);

	effect->Begin(NULL, 0);
	effect->BeginPass(0);
	{
		object3->Draw(flags, effect);
	}
	effect->EndPass();
	effect->End();

	// car
	if( drawcar )
	{
		D3DXMatrixInverse(&inv, NULL, &world[0]);

		effect->SetMatrix("matWorld", &world[0]);
		effect->SetMatrix("matWorldInv", &inv);

		if( drawforfinal )
		{
			// draw other parts normally
			effect->Begin(NULL, 0);
			effect->BeginPass(0);
			{
				object2->DrawExcept(6, flags, effect);
			}
			effect->EndPass();
			effect->End();

			// draw chassis with reflection
			effect->SetTechnique("deferredlight_fresnel");
			effect->SetVector("eyePos", (D3DXVECTOR4*)&eye);

			effect->Begin(NULL, 0);
			effect->BeginPass(0);
			{
				device->SetTexture(3, envcube);
				object2->DrawSubset(6, flags, effect);
				device->SetTexture(3, NULL);
			}
			effect->EndPass();
			effect->End();

			effect->SetTechnique(oldtech);
		}
		else
		{
			// draw as-is
			effect->Begin(NULL, 0);
			effect->BeginPass(0);
			{
				if( drawforfinal )
				{
					device->SetTexture(3, envcube);
					object2->Draw(flags, effect);
					device->SetTexture(3, NULL);
				}
				else
					object2->Draw(flags, effect);
			}
			effect->EndPass();
			effect->End();
		}
	}

	// bridge
	if( drawforgbuffer )
	{
		effect->SetTechnique("gbuffer_tbn_dlighting");

		// no need to flip binormal (was exported)
		params = D3DXVECTOR4(1, 1, 1, 1);
		effect->SetVector("params", &params);
	}

	D3DXMatrixInverse(&inv, NULL, &world[1]);

	effect->SetMatrix("matWorldInv", &inv);
	effect->SetMatrix("matWorld", &world[1]);

	effect->Begin(NULL, 0);
	effect->BeginPass(0);
	{
		if( drawforgbuffer )
		{
			device->SetTexture(0, texture3); // spec power
			device->SetTexture(1, texture4); // normal map
		}

		object1->Draw(flags, effect);
	}
	effect->EndPass();
	effect->End();

	if( drawforgbuffer )
		effect->SetTechnique(oldtech);
}
//*************************************************************************************************************
void DrawSceneForShadow(LPD3DXEFFECT effect)
{
	// gate
	if( drawgate )
	{
		effect->SetMatrix("matWorld", &world[2]);
		effect->CommitChanges();

		object3->Draw(DXObject::Opaque);
	}

	// car
	effect->SetMatrix("matWorld", &world[0]);
	effect->CommitChanges();

	object2->Draw(DXObject::Opaque);

	// bridge
	distance->SetMatrix("matWorld", &world[1]);
	distance->CommitChanges();

	object1->Draw(DXObject::Opaque);
}
//*************************************************************************************************************
void DrawShadowMaps()
{
	LPDIRECT3DSURFACE9	oldsurface = NULL;
	device->GetRenderTarget(0, &oldsurface);

	// directional lights
	if( currentlight == 0 )
	{
		distance->SetTechnique("distance_directional");
		distance->Begin(NULL, 0);
		distance->BeginPass(0);
		{
			for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
			{
				DXDirectionalLight& lt = directionallights[i];
				lt.DrawShadowMap(device, distance, &DrawSceneForShadow);
			}
		}
		distance->EndPass();
		distance->End();
	}

	// point lights
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	drawgate = false;

	distance->SetTechnique("distance_point");
	distance->Begin(NULL, 0);
	distance->BeginPass(0);
	{
		for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
		{
			if( currentlight > 0 && ((i + 1) != currentlight) )
				continue;

			DXPointLight& lt = pointlights[i];
			lt.DrawShadowMap(device, distance, &DrawSceneForShadow);
		}
	}
	distance->EndPass();
	distance->End();

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	drawgate = true;

	// now blur them
	BlurShadowMaps();

	device->SetRenderTarget(0, oldsurface);
	oldsurface->Release();
}
//*************************************************************************************************************
void BlurShadowMaps()
{
	device->SetRenderState(D3DRS_ZENABLE, FALSE);

	// blur directional shadows
	if( currentlight == 0 )
	{
		distance->SetTechnique("blur5x5");
		distance->Begin(NULL, 0);
		distance->BeginPass(0);
		{
			for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
			{
				DXDirectionalLight& lt = directionallights[i];
				lt.BlurShadowMap(device, distance);
			}
		}
		distance->EndPass();
		distance->End();
	}

	// blur point shadows
	distance->SetTechnique("blurcube5x5");
	distance->Begin(NULL, 0);
	distance->BeginPass(0);
	{
		for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
		{
			DXPointLight& lt = pointlights[i];

			if( currentlight > 0 && ((i + 1) != currentlight) )
				continue;

			lt.BlurShadowMap(device, distance);
		}
	}
	distance->EndPass();
	distance->End();

	device->SetRenderState(D3DRS_ZENABLE, TRUE);
}
//*************************************************************************************************************
void DrawEnvironment()
{
	LPDIRECT3DSURFACE9 oldtarget = NULL;
	LPDIRECT3DSURFACE9 surface = NULL;
	ViewParams params;

	params.eye				= D3DXVECTOR3(0, -0.15f, 0);
	params.viewport.X		= 0;
	params.viewport.Y		= 0;
	params.viewport.Width	= CUBEMAP_SIZE;
	params.viewport.Height	= CUBEMAP_SIZE;
	params.viewport.MinZ	= 0;
	params.viewport.MaxZ	= 1;

	D3DXMatrixPerspectiveFovLH(&params.proj, D3DX_PI / 2, 1, 0.1f, 100.0f);
	device->GetRenderTarget(0, &oldtarget);

	drawcar = false;

	for( DWORD i = 0; i < 6; ++i )
	{
		DXGetCubemapViewMatrix(params.view, i, params.eye);
		envcube->GetCubeMapSurface((D3DCUBEMAP_FACES)i, 0, &surface);

		DrawDeferred(surface, params);
		surface->Release();
	}

	device->SetRenderTarget(0, oldtarget);
	oldtarget->Release();

	drawcar = true;
}
//*************************************************************************************************************
void DrawDeferred(LPDIRECT3DSURFACE9 target, const ViewParams& params)
{
	D3DXMATRIX	inv;
	D3DXMATRIX	viewproj;
	D3DXMATRIX	shadowvp;
	D3DXVECTOR4	lightpos;
	D3DXVECTOR4	scale((float)params.viewport.Width / screenwidth, (float)params.viewport.Height / screenheight, 0, 1);
	RECT		scissorbox;

	// TODO: x, y
	quadvertices[6] = quadvertices[24] = quadvertices[30] = (float)params.viewport.Width - 0.5f;
	quadvertices[13] = quadvertices[19] = quadvertices[31] = (float)params.viewport.Height - 0.5f;

	D3DXMatrixMultiply(&viewproj, &params.view, &params.proj);

	// STEP 1: geometry pass
	device->SetRenderTarget(0, normalsurf);
	device->SetRenderTarget(1, depthsurf);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
	device->SetViewport(&params.viewport);

	DrawScene(rendergbuffer, viewproj, params.eye);

	// STEP 2: deferred pass
	D3DXMatrixInverse(&inv, NULL, &viewproj);

	deferred->SetVector("eyePos", (D3DXVECTOR4*)&params.eye);
	deferred->SetMatrix("matViewProjInv", &inv);
	deferred->SetVector("scale", &scale);

	device->SetRenderTarget(0, irradsurf1);
	device->SetRenderTarget(1, irradsurf2);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
	device->SetViewport(&params.viewport);

	device->SetTexture(1, normalpower);
	device->SetTexture(3, depth);

	device->SetVertexDeclaration(quaddecl);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	// directional lights
	if( currentlight == 0 )
	{
		deferred->SetTechnique("irradiance_directional");

		deferred->Begin(NULL, 0);
		deferred->BeginPass(0);
		{
			for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
			{
				DXDirectionalLight& lt = directionallights[i];
				lt.GetViewProjMatrix(shadowvp, D3DXVECTOR3(0, 0, 0));

				deferred->SetVector("lightColor", (D3DXVECTOR4*)&lt.GetColor());
				deferred->SetVector("lightPos", &lt.GetDirection());
				deferred->SetMatrix("lightViewProj", &shadowvp);
				deferred->CommitChanges();

				device->SetTexture(2, lt.GetShadowMap());
				device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, 6 * sizeof(float));
			}
		}
		deferred->EndPass();
		deferred->End();
	}

	// point lights
	deferred->SetTechnique("irradiance_point");
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

	deferred->Begin(NULL, 0);
	deferred->BeginPass(0);
	{
		for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
		{
			DXPointLight& lt = pointlights[i];

			if( currentlight > 0 && ((i + 1) != currentlight) )
				continue;

			lightpos = D3DXVECTOR4(lt.GetPosition(), lt.GetRadius());

			deferred->SetVector("lightColor", (D3DXVECTOR4*)&lt.GetColor());
			deferred->SetVector("lightPos", &lightpos);
			deferred->CommitChanges();

			lt.GetScissorRect(scissorbox, params.view, params.proj, params.viewport.Width, params.viewport.Height);

			device->SetTexture(2, lt.GetShadowMap());
			device->SetScissorRect(&scissorbox);
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, 6 * sizeof(float));
		}
	}
	deferred->EndPass();
	deferred->End();

	device->SetTexture(2, NULL);

	// spot lights
	deferred->SetTechnique("irradiance_spot");

	deferred->Begin(NULL, 0);
	deferred->BeginPass(0);
	{
		for( int i = 0; i < NUM_SPOTLIGHTS; ++i )
		{
			DXSpotLight& lt = spotlights[i];
			lightpos = D3DXVECTOR4(lt.GetPosition(), lt.GetRadius());

			deferred->SetVector("lightColor", (D3DXVECTOR4*)&lt.GetColor());
			deferred->SetVector("lightPos", &lightpos);
			deferred->SetVector("spotDir", (D3DXVECTOR4*)&lt.GetDirection());
			deferred->SetVector("spotParams", &lt.GetParams());
			deferred->CommitChanges();

			lt.GetScissorRect(scissorbox, params.view, params.proj, params.viewport.Width, params.viewport.Height);

			device->SetScissorRect(&scissorbox);
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, 6 * sizeof(float));
		}
	}
	deferred->EndPass();
	deferred->End();

	device->SetTexture(1, NULL);
	device->SetTexture(3, NULL);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	// render sky
	device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

	device->SetRenderTarget(0, target);
	device->SetRenderTarget(1, NULL);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff6694ed, 1.0f, 0);

	D3DXMatrixScaling(&inv, 20, 20, 20);
	skyeffect->SetMatrix("matWorld", &inv);

	D3DXMatrixRotationX(&inv, D3DXToRadian(-30));
	skyeffect->SetMatrix("matWorldSky", &inv);

	skyeffect->SetMatrix("matViewProj", &viewproj);
	skyeffect->SetVector("eyePos", (D3DXVECTOR4*)&params.eye);

	skyeffect->Begin(0, 0);
	skyeffect->BeginPass(0);
	{
		device->SetTexture(0, skytex);
		skymesh->DrawSubset(0);
	}
	skyeffect->EndPass();
	skyeffect->End();

	// STEP 3: final scene pass
	finalpass->SetVector("scale", &scale);

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
	device->SetRenderState(D3DRS_ZENABLE, TRUE);
	device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

	device->SetTexture(1, irradiance1);
	device->SetTexture(2, irradiance2);

	DrawScene(finalpass, viewproj, params.eye);

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

	device->SetTexture(1, NULL);
	device->SetTexture(2, NULL);
}
//*************************************************************************************************************
void DrawTransparent(const ViewParams& params)
{
	D3DXMATRIX inv;
	D3DXMATRIX viewproj;
	D3DXMATRIX shadowvp;
	D3DXVECTOR4 lightpos;
	bool firstpass = true;

	D3DXMatrixMultiply(&viewproj, &params.view, &params.proj);
	D3DXMatrixInverse(&inv, NULL, &world[0]);

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	if( currentlight == 0 )
	{
		// directional lights
		forward->SetTechnique("forward_directional");
		forward->SetMatrix("matWorld", &world[0]);
		forward->SetMatrix("matWorldInv", &inv);
		forward->SetMatrix("matViewProj", &viewproj);
		forward->SetVector("eyePos", (D3DXVECTOR4*)&params.eye);

		forward->Begin(NULL, 0);
		forward->BeginPass(0);
		{
			for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
			{
				DXDirectionalLight& lt = directionallights[i];
				lt.GetViewProjMatrix(shadowvp, D3DXVECTOR3(0, 0, 0));

				forward->SetVector("lightColor", (D3DXVECTOR4*)&lt.GetColor());
				forward->SetVector("lightPos", &lt.GetDirection());
				forward->SetMatrix("lightViewProj", &shadowvp);
				forward->CommitChanges();

				device->SetTexture(2, lt.GetShadowMap());
				object2->Draw(DXObject::Transparent|DXObject::Material, forward);

				if( firstpass )
				{
					firstpass = false;

					device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
					device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				}
			}
		}
		forward->EndPass();
		forward->End();
	}

	// point lights
	forward->SetTechnique("forward_point");

	forward->Begin(NULL, 0);
	forward->BeginPass(0);
	{
		for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
		{
			DXPointLight& lt = pointlights[i];

			if( currentlight > 0 && ((i + 1) != currentlight) )
				continue;

			lightpos = D3DXVECTOR4(lt.GetPosition(), lt.GetRadius());

			forward->SetVector("lightColor", (D3DXVECTOR4*)&lt.GetColor());
			forward->SetVector("lightPos", &lightpos);
			forward->CommitChanges();

			device->SetTexture(2, lt.GetShadowMap());
			object2->Draw(DXObject::Transparent|DXObject::Material, forward);

			if( firstpass )
			{
				firstpass = false;

				device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			}
		}
	}
	forward->EndPass();
	forward->End();

	device->SetTexture(2, NULL);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	// camera
	D3DXVECTOR3 look(-1, 0.3f, -0.4f);
	D3DXVECTOR3 up(0, 1, 0);
	ViewParams params;

	params.eye = D3DXVECTOR3(-2.5f, 1.26f, -2.0f);

	D3DXMatrixPerspectiveFovLH(&params.proj, D3DX_PI / 4, (float)screenwidth / (float)screenheight, 0.1f, 100);
	D3DXMatrixLookAtLH(&params.view, &params.eye, &look, &up);

	// lights
	pointlights[0].GetPosition().x = cosf(time * 0.5f) * 2;
	pointlights[0].GetPosition().z = sinf(time * 0.5f) * cosf(time * 0.5f) * 2;

	pointlights[1].GetPosition().x = cosf(1.5f * time) * 2;
	pointlights[1].GetPosition().z = sinf(1 * time) * 2;

	pointlights[2].GetPosition().x = cosf(0.75f * time) * 1.5f;
	pointlights[2].GetPosition().z = sinf(1.5f * time) * 1.5f;

	time += elapsedtime;

	// world matrices
	D3DXVECTOR3 sc(0.008f, 0.008f, 0.008f);
	D3DXQUATERNION rot;

	D3DXMatrixScaling(&world[0], 0.5f, 0.5f, 0.5f);
	D3DXMatrixScaling(&world[1], 1, 1, 1);

	D3DXQuaternionRotationYawPitchRoll(&rot, D3DX_PI / 2, 0, 0);
	D3DXMatrixTransformation(&world[2], NULL, NULL, &sc, NULL, &rot, NULL);

	world[2]._41 = sinf(1.5f * time) * 2.0f;
	world[2]._42 = 0.3f;
	world[2]._43 = 1.3f;

	world[0]._41 = 0;
	world[0]._42 = -0.25f;
	world[0]._43 = 0;

	world[1]._42 = -1;

	// render
	if( SUCCEEDED(device->BeginScene()) )
	{
		device->GetViewport(&params.viewport);

		// shadow pass
		DrawShadowMaps();

		// environment cubemap
		DrawEnvironment();

		// draw scene & lights
		LPDIRECT3DSURFACE9 backbuffer = NULL;
		device->GetRenderTarget(0, &backbuffer);

		DrawDeferred(backbuffer, params);
		backbuffer->Release();

		DrawTransparent(params);

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
