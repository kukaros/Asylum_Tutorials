//*************************************************************************************************************
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "GdiPlus.lib")

#include <d3dx9.h>
#include <iostream>

// helper macros
#define MYERROR(x)		{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)		{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }

// external variables
extern long screenwidth;
extern long screenheight;

extern LPDIRECT3DDEVICE9				device;
extern D3DPRESENT_PARAMETERS			d3dpp;
extern LPD3DXEFFECT						rendergbuffer;
extern LPD3DXEFFECT						deferred;
extern LPD3DXEFFECT						forward;
extern LPDIRECT3DVERTEXDECLARATION9		vertexdecl;
extern LPD3DXMESH						mesh2;
extern LPD3DXMESH						mesh1;

extern LPDIRECT3DTEXTURE9				texture1;
extern LPDIRECT3DTEXTURE9				texture2;
extern LPDIRECT3DTEXTURE9				texture3;
extern LPDIRECT3DTEXTURE9				albedo;
extern LPDIRECT3DTEXTURE9				normaldepth;
extern LPDIRECT3DTEXTURE9				scene;
extern LPDIRECT3DTEXTURE9				text;

extern LPDIRECT3DSURFACE9				albedosurf;
extern LPDIRECT3DSURFACE9				normaldepthsurf;
extern LPDIRECT3DSURFACE9				scenesurf;

// external functions
HRESULT DXCreateEffect(const char* file, LPD3DXEFFECT* out);
void PreRenderText(const std::string& str, LPDIRECT3DTEXTURE9 tex, DWORD width, DWORD height);

// tutorial variables
D3DXVECTOR3		eye(-3, 3, -3);
D3DXVECTOR4		scenesize(2.0f / (float)screenwidth, 2.0f / (float)screenheight, 0, 1);
D3DXMATRIX		view, proj, viewproj;
D3DXMATRIX		world[4];
int				currentlight = 0;

void DrawScene();
void DrawForward();
void DrawDeferred();

class PointLight
{
public:
	D3DXVECTOR4		Color;
	D3DXVECTOR3		Position;
	float			Radius;

	void GetScissorRect(RECT& out, const D3DXVECTOR3& eye, const D3DXMATRIX& view, const D3DXMATRIX& proj) const;
};

class DirectionalLight
{
public:
	D3DXVECTOR4		Color;
	D3DXVECTOR3		Direction;
};

PointLight pointlights[] =
{
	{ D3DXCOLOR(1, 0, 0, 1), D3DXVECTOR3(1.5f, 0.5f, 0), 2 },
	{ D3DXCOLOR(0, 1, 0, 1), D3DXVECTOR3(-0.7f, 0.5f, 1.2f), 2 },
	{ D3DXCOLOR(0, 0, 1, 1), D3DXVECTOR3(0, 0.5f, 0), 2 },
};

DirectionalLight directionallights[] =
{
	{ D3DXCOLOR(0.1f, 0.1f, 0.2f, 1), D3DXVECTOR3(-1, 1, 1) }
};

static const int NUM_POINT_LIGHTS = sizeof(pointlights) / sizeof(pointlights[0]);
static const int NUM_DIRECTIONAL_LIGHTS = sizeof(directionallights) / sizeof(directionallights[0]);

float vertices[36] =
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
	(float)screenwidth - 522.5f,			9.5f,	0, 1,	0, 0,
	(float)screenwidth - 10.5f,				9.5f,	0, 1,	1, 0,
	(float)screenwidth - 522.5f,	128.0f + 9.5f,	0, 1,	0, 1,

	(float)screenwidth - 522.5f,	128.0f + 9.5f,	0, 1,	0, 1,
	(float)screenwidth - 10.5f,				9.5f,	0, 1,	1, 0,
	(float)screenwidth - 5.5f,		128.0f + 9.5f,	0, 1,	1, 1
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

	MYVALID(D3DXLoadMeshFromX("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh2));
	MYVALID(D3DXLoadMeshFromX("../media/meshes/skullocc3.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2.jpg", &texture2));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/static_sky.jpg", &texture3));

	MYVALID(device->CreateTexture(512, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &albedo, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &scene, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &normaldepth, NULL));
	MYVALID(device->CreateVertexDeclaration(elem, &vertexdecl));

	MYVALID(DXCreateEffect("../media/shaders/gbuffer.fx", &rendergbuffer));
	MYVALID(DXCreateEffect("../media/shaders/deferred.fx", &deferred));
	MYVALID(DXCreateEffect("../media/shaders/forward.fx", &forward));

	MYVALID(albedo->GetSurfaceLevel(0, &albedosurf));
	MYVALID(normaldepth->GetSurfaceLevel(0, &normaldepthsurf));
	MYVALID(scene->GetSurfaceLevel(0, &scenesurf));

	deferred->SetTechnique("deferred");

	PreRenderText("Use the 1-3 buttons to view one light only\nUse the 0 button to view all lights", text, 512, 128);

	// setup camera
	D3DXVECTOR3 look(0, 0.5f, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)d3dpp.BackBufferWidth / (float)d3dpp.BackBufferHeight, 0.1f, 100);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixMultiply(&viewproj, &view, &proj);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	if( wparam >= 0x30 && wparam <= 0x39 )
		currentlight = wparam - 0x30;
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

		// table
		D3DXMatrixInverse(&inv, NULL, &world[3]);

		effect->SetMatrix("matWorldInv", &inv);
		effect->SetMatrix("matWorld", &world[3]);
		effect->CommitChanges();

		device->SetTexture(0, texture2);
		mesh2->DrawSubset(0);
	}
	effect->EndPass();
	effect->End();
}
//*************************************************************************************************************
void DrawForward()
{
	LPDIRECT3DSURFACE9	oldsurface = NULL;
	D3DXVECTOR4 lightpos;

	// fill zbuffer
	device->GetRenderTarget(0, &oldsurface);
	device->SetRenderTarget(0, scenesurf);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
	device->SetRenderState(D3DRS_COLORWRITEENABLE, 0);

	forward->SetTechnique("zonly");
	DrawScene(forward);

	// render geometry
	device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA);
	device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	forward->SetTechnique("forward");
	forward->SetVector("eyePos", (D3DXVECTOR4*)&eye);

	for( int i = 0; i < NUM_POINT_LIGHTS; ++i )
	{
		const PointLight& lt = pointlights[i];

		lightpos = D3DXVECTOR4(lt.Position, lt.Radius);

		forward->SetVector("lightColor", &lt.Color);
		forward->SetVector("lightPos", &lightpos);

		DrawScene(forward);
	}

	device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

	// combine with sky & gamma correct
	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderTarget(0, oldsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff6694ed, 1.0f, 0);

	oldsurface->Release();

	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);

	device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
	device->SetTexture(0, scene);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));

	forward->End();

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_ZENABLE, TRUE);
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
	device->SetRenderTarget(0, scenesurf);
	device->SetRenderTarget(1, NULL);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);

	device->SetTexture(0, albedo);
	device->SetTexture(1, normaldepth);

	device->SetVertexDeclaration(vertexdecl);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);

	D3DXMatrixInverse(&inv, NULL, &viewproj);

	deferred->SetVector("eyePos", (D3DXVECTOR4*)&eye);
	deferred->SetMatrix("matViewProjInv", &inv);

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

				lightpos = D3DXVECTOR4(lt.Direction, 0);

				deferred->SetVector("lightColor", &lt.Color);
				deferred->SetVector("lightPos", &lightpos);
				deferred->CommitChanges();

				device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
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

			lt.GetScissorRect(scissorbox, eye, view, proj);

			device->SetScissorRect(&scissorbox);
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
	}
	deferred->EndPass();
	deferred->End();

	device->SetTexture(1, NULL);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	// combine with sky & gamma correct
	device->SetRenderTarget(0, oldsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0); //0xff6694ed

	oldsurface->Release();

	device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
	device->SetTexture(0, texture3);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));

	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);

	device->SetTexture(0, scene);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));

	device->SetTexture(0, text);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, textvertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_ZENABLE, TRUE);
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

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
	D3DXMatrixScaling(&world[3], 5, 0.1f, 5);

	world[0]._41 = -1.5;
	world[0]._43 = 1.5;

	world[1]._41 = 1.5;
	world[1]._43 = 1.5;

	world[2]._41 = 0;
	world[2]._43 = -1;

	// render
	if( SUCCEEDED(device->BeginScene()) )
	{
		//DrawForward();
		DrawDeferred();

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
void PointLight::GetScissorRect(RECT& out, const D3DXVECTOR3& eye, const D3DXMATRIX& view, const D3DXMATRIX& proj) const
{
	D3DXVECTOR3 vcenter;
	D3DXVECTOR3 s0, s3;
	D3DXVECTOR4 c0, c3;

	float dist = D3DXVec3Length(&(eye - Position));

	D3DXVec3TransformCoord(&vcenter, &Position, &view);

	s0 = vcenter + D3DXVECTOR3(-1, -1, 0) * Radius;
	s3 = vcenter + D3DXVECTOR3(1, 1, 0) * Radius;

	D3DXVec3Transform(&c0, &s0, &proj);
	D3DXVec3Transform(&c3, &s3, &proj);

	c0 /= c0.w;
	c3 /= c3.w;

	if( dist < Radius )
	{
		out.left	= 0;
		out.top		= 0;
		out.right	= screenwidth;
		out.bottom	= screenheight;
	}
	else
	{
		out.left	= (LONG)((c0.x * 0.5f + 0.5f) * screenwidth);
		out.top		= (LONG)((c0.y * -0.5f + 0.5f) * screenheight);
		out.right	= (LONG)((c3.x * 0.5f + 0.5f) * screenwidth);
		out.bottom	= (LONG)((c3.y * -0.5f + 0.5f) * screenheight);
	}
}
//*************************************************************************************************************
