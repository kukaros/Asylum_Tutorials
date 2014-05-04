//*************************************************************************************************************
#include <iostream>

#include "../common/dxext.h"

// helper macros
#define TITLE				"Shader tutorial 15: Deferred shading"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

#define SHADOWMAP_SIZE		512
#define CUBESHADOWMAP_SIZE	256

// external variables
extern long screenwidth;
extern long screenheight;
extern LPDIRECT3DDEVICE9 device;
extern HWND hwnd;

// tutorial variables
LPD3DXEFFECT					rendergbuffer	= NULL;
LPD3DXEFFECT					deferred		= NULL;
LPD3DXEFFECT					distance		= NULL;
LPDIRECT3DVERTEXDECLARATION9	quaddecl		= NULL;

LPD3DXMESH						mesh1			= NULL;
LPD3DXMESH						mesh2			= NULL;
LPDIRECT3DTEXTURE9				texture1		= NULL;
LPDIRECT3DTEXTURE9				texture2		= NULL;
LPDIRECT3DTEXTURE9				texture3		= NULL;
LPDIRECT3DTEXTURE9				texture4		= NULL;

LPDIRECT3DTEXTURE9				albedo			= NULL;
LPDIRECT3DTEXTURE9				normals			= NULL;
LPDIRECT3DTEXTURE9				depth			= NULL;
LPDIRECT3DTEXTURE9				scene			= NULL;
LPDIRECT3DTEXTURE9				text			= NULL;

LPDIRECT3DSURFACE9				albedosurf		= NULL;
LPDIRECT3DSURFACE9				normalsurf		= NULL;
LPDIRECT3DSURFACE9				depthsurf		= NULL;
LPDIRECT3DSURFACE9				scenesurf		= NULL;

D3DXVECTOR3		eye(-3, 3, -3);
D3DXMATRIX		view, proj, viewproj;
D3DXMATRIX		world[4];
int				currentlight = 0;

DXPointLight pointlights[] =
{
	DXPointLight(D3DXCOLOR(1, 0, 0, 1), D3DXVECTOR3(1.5f, 0.5f, 0), 5),
	DXPointLight(D3DXCOLOR(0, 1, 0, 1), D3DXVECTOR3(-0.7f, 0.5f, 1.2f), 5),
	DXPointLight(D3DXCOLOR(0, 0, 1, 1), D3DXVECTOR3(0, 0.5f, 0), 5),
};

DXDirectionalLight directionallights[] =
{
	DXDirectionalLight(D3DXCOLOR(0.6f, 0.6f, 1, 1), D3DXVECTOR4(0, 0, 0, 5)),
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

static const int NUM_POINT_LIGHTS = sizeof(pointlights) / sizeof(pointlights[0]);
static const int NUM_DIRECTIONAL_LIGHTS = sizeof(directionallights) / sizeof(directionallights[0]);

// tutorial functions
void DrawScene(LPD3DXEFFECT effect);
void DrawSceneForShadow(LPD3DXEFFECT effect);
void DrawDeferred();
void DrawShadowMaps();
void BlurShadowMaps();

HRESULT InitScene()
{
	HRESULT hr;
	D3DCAPS9 caps;

	SetWindowText(hwnd, TITLE);

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

	MYVALID(D3DXLoadMeshFromX("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh2));
	MYVALID(D3DXLoadMeshFromX("../media/meshes/skullocc3.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2.jpg", &texture2));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/static_sky.jpg", &texture3));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2_normal.tga", &texture4));

	MYVALID(device->CreateTexture(512, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &albedo, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &scene, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &normals, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &depth, NULL));

	MYVALID(DXCreateEffect("../media/shaders/gbuffer.fx", device, &rendergbuffer));
	MYVALID(DXCreateEffect("../media/shaders/deferred.fx", device, &deferred));
	MYVALID(DXCreateEffect("../media/shaders/distance.fx", device, &distance));

	MYVALID(albedo->GetSurfaceLevel(0, &albedosurf));
	MYVALID(normals->GetSurfaceLevel(0, &normalsurf));
	MYVALID(scene->GetSurfaceLevel(0, &scenesurf));
	MYVALID(depth->GetSurfaceLevel(0, &depthsurf));

	for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
		pointlights[i].CreateShadowMap(device, DXLight::Dynamic, CUBESHADOWMAP_SIZE);

	for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
		directionallights[i].CreateShadowMap(device, DXLight::Dynamic, SHADOWMAP_SIZE);

	rendergbuffer->SetTechnique("gbuffer_dshading");

	MYVALID(DXGenTangentFrame(device, &mesh2));
	DXRenderText("Press buttons 0-3 to view lights", text, 512, 128);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	// don't do this ever!
	for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
		pointlights[i].~DXPointLight();

	for( int i = 0; i < NUM_DIRECTIONAL_LIGHTS; ++i )
		directionallights[i].~DXDirectionalLight();

	SAFE_RELEASE(scenesurf);
	SAFE_RELEASE(normalsurf);
	SAFE_RELEASE(depthsurf);
	SAFE_RELEASE(albedosurf);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(texture2);
	SAFE_RELEASE(texture3);
	SAFE_RELEASE(texture4);
	SAFE_RELEASE(scene);
	SAFE_RELEASE(text);
	SAFE_RELEASE(normals);
	SAFE_RELEASE(depth);
	SAFE_RELEASE(albedo);
	SAFE_RELEASE(quaddecl);
	SAFE_RELEASE(mesh2);
	SAFE_RELEASE(mesh1);
	SAFE_RELEASE(rendergbuffer);
	SAFE_RELEASE(deferred);
	SAFE_RELEASE(distance);

	DXKillAnyRogueObject();
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
	D3DXHANDLE tech = effect->GetTechniqueByName("gbuffer_tbn_dshading");
	D3DXVECTOR4 params(1, 1, 1, 1);

	D3DXMatrixInverse(&inv, NULL, &world[0]);

	effect->SetMatrix("matWorld", &world[0]);
	effect->SetMatrix("matWorldInv", &inv);
	effect->SetMatrix("matViewProj", &viewproj);
	effect->SetVector("params", &params);

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
	{
		effect->SetTechnique(tech);

		// repeat wood texture and flip binormal
		params = D3DXVECTOR4(2, 2, -1, 1);
		effect->SetVector("params", &params);
	}

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
			if( currentlight > 0 && ((i + 1) != currentlight) )
				continue;

			DXPointLight& lt = pointlights[i];
			lt.BlurShadowMap(device, distance);
		}
	}
	distance->EndPass();
	distance->End();

	device->SetRenderState(D3DRS_ZENABLE, TRUE);
}
//*************************************************************************************************************
void DrawDeferred()
{
	LPDIRECT3DSURFACE9	oldsurface = NULL;
	D3DXMATRIX			inv;
	D3DXMATRIX			shadowvp;
	D3DXVECTOR4			lightpos;
	RECT				scissorbox;

	// geometry pass
	device->GetRenderTarget(0, &oldsurface);
	device->SetRenderTarget(0, albedosurf);
	device->SetRenderTarget(1, normalsurf);
	device->SetRenderTarget(2, depthsurf);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);

	DrawScene(rendergbuffer);

	// deferred pass
	D3DXMatrixInverse(&inv, NULL, &viewproj);

	deferred->SetVector("eyePos", (D3DXVECTOR4*)&eye);
	deferred->SetMatrix("matViewProjInv", &inv);

	device->SetRenderTarget(0, scenesurf);
	device->SetRenderTarget(1, NULL);
	device->SetRenderTarget(2, NULL);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);

	device->SetTexture(0, albedo);
	device->SetTexture(1, normals);
	device->SetTexture(3, depth);

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
	deferred->SetTechnique("deferred_point");

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

			lt.GetScissorRect(scissorbox, view, proj, screenwidth, screenheight);

			device->SetTexture(2, lt.GetShadowMap());
			device->SetScissorRect(&scissorbox);
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, 6 * sizeof(float));
		}
	}
	deferred->EndPass();
	deferred->End();

	device->SetTexture(1, NULL);
	device->SetTexture(2, NULL);
	device->SetTexture(3, NULL);

	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	// combine with sky & gamma correct
	device->SetRenderTarget(0, oldsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

	oldsurface->Release();

	device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
	device->SetTexture(0, texture3);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, 6 * sizeof(float));

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);

	device->SetTexture(0, scene);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, 6 * sizeof(float));

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

	// render text
	device->SetTexture(0, text);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, textvertices, 6 * sizeof(float));

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

	directionallights[0].GetDirection().x = dir.x;
	directionallights[0].GetDirection().y = dir.y;
	directionallights[0].GetDirection().z = dir.z;

	pointlights[0].GetPosition().x = cosf(time * 0.5f) * 2;
	pointlights[0].GetPosition().z = sinf(time * 0.5f) * cosf(time * 0.5f) * 2;

	pointlights[1].GetPosition().x = cosf(1.5f * time) * 2;
	pointlights[1].GetPosition().z = sinf(1 * time) * 2;

	pointlights[2].GetPosition().x = cosf(0.75f * time) * 1.5f;
	pointlights[2].GetPosition().z = sinf(1.5f * time) * 1.5f;

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
		DrawShadowMaps();

		// draw scene & lights
		D3DXMatrixScaling(&world[3], 5, 0.1f, 5);
		DrawDeferred();

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
