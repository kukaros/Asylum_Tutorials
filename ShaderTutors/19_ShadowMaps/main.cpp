//*************************************************************************************************************
#include <iostream>
#include <string>

#include "../common/dxext.h"

#define SHADOWMAP_SIZE	512

// object types
#define FLOOR	1
#define CRATE	3
#define SKULL	2

// helper macros
#define TITLE				"Shader sample 19: Shadow map techniques"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern long		screenwidth;
extern long		screenheight;
extern short	mousedx;
extern short	mousedy;
extern short	mousedown;

extern LPDIRECT3DDEVICE9 device;
extern HWND hwnd;

// sample structures
struct SceneObject
{
	D3DXMATRIX	world;
	D3DXVECTOR3	position;
	D3DXVECTOR3	scale;
	D3DXVECTOR3	angles;
	int			type;
	int			behavior;	// 1 = caster, 2 = receiver
};

// sample variables
LPD3DXMESH						skull		= NULL;
LPD3DXMESH						box			= NULL;
LPD3DXEFFECT					convolution	= NULL;
LPD3DXEFFECT					boxblur5x5	= NULL;
LPDIRECT3DVERTEXDECLARATION9	vertexdecl	= NULL;
LPDIRECT3DTEXTURE9				texture1	= NULL;
LPDIRECT3DTEXTURE9				texture2	= NULL;
LPDIRECT3DTEXTURE9				texture3	= NULL;
LPDIRECT3DTEXTURE9				shadowmap	= NULL;
LPDIRECT3DTEXTURE9				sincoeffs	= NULL;
LPDIRECT3DTEXTURE9				coscoeffs	= NULL;
LPDIRECT3DTEXTURE9				blur		= NULL;
LPDIRECT3DTEXTURE9				text		= NULL;

state<D3DXVECTOR2>				cameraangle;

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

SceneObject objects[] =
{
	{ D3DXMATRIX(), D3DXVECTOR3(0, 0.05f, 1.2f), D3DXVECTOR3(0.2f, 0.2f, 0.2f), D3DXVECTOR3(D3DX_PI / 3, 0, 0), SKULL, 3 },
	{ D3DXMATRIX(), D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(5, 0.1f, 5), D3DXVECTOR3(0, 0, 0), FLOOR, 3 },

	{ D3DXMATRIX(), D3DXVECTOR3(0.5f, 0.38f, -1), D3DXVECTOR3(1.2f, 0.75f, 0.75f), D3DXVECTOR3(-0.3f, 0, 0), CRATE, 3 },
	{ D3DXMATRIX(), D3DXVECTOR3(0.55f, 1.005f, -0.85f), D3DXVECTOR3(0.5f, 0.5f, 0.5f), D3DXVECTOR3(-0.3f, 0, 0), CRATE, 3 },

	{ D3DXMATRIX(), D3DXVECTOR3(-1.75f, 0.38f, 0.4f), D3DXVECTOR3(0.75f, 0.75f, 1.5f), D3DXVECTOR3(-0.15f, 0, 0), CRATE, 3 },
};

const int numobjects = sizeof(objects) / sizeof(objects[0]);

HRESULT InitScene()
{
	HRESULT hr;
	
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	SetWindowText(hwnd, TITLE);

	MYVALID(D3DXLoadMeshFromX("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &box));
	MYVALID(D3DXLoadMeshFromX("../media/meshes/skullocc3.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skull));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2.jpg", &texture2));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/crate.jpg", &texture3));

	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &shadowmap, NULL));
	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &sincoeffs, NULL));
	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &coscoeffs, NULL));
	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &blur, NULL));
	
	MYVALID(device->CreateTexture(512, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));
	MYVALID(device->CreateVertexDeclaration(elem, &vertexdecl));

	MYVALID(DXCreateEffect("../media/shaders/convolutionshadow.fx", device, &convolution));
	MYVALID(DXCreateEffect("../media/shaders/boxblur5x5.fx", device, &boxblur5x5));

	D3DXVECTOR4 noisesize(16.0f, 16.0f, 0, 1);
	D3DXVECTOR4 texelsize(1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE, 0, 1);

	DXRenderText("Use mouse to rotate camera", text, 512, 128);
	cameraangle = D3DXVECTOR2(0.78f, 0.78f);

	D3DXMATRIX tmp1, tmp2, tmp3;

	for( int i = 0; i < numobjects; ++i )
	{
		SceneObject& obj = objects[i];

		D3DXMatrixScaling(&tmp1, obj.scale.x, obj.scale.y, obj.scale.z);
		D3DXMatrixRotationYawPitchRoll(&tmp2, obj.angles.x, obj.angles.y, obj.angles.z);
		D3DXMatrixTranslation(&tmp3, obj.position.x, obj.position.y, obj.position.z);

		D3DXMatrixMultiply(&obj.world, &tmp1, &tmp2);
		D3DXMatrixMultiply(&obj.world, &obj.world, &tmp3);
	}

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_RELEASE(convolution);
	SAFE_RELEASE(boxblur5x5);
	SAFE_RELEASE(vertexdecl);
	SAFE_RELEASE(skull);
	SAFE_RELEASE(box);
	SAFE_RELEASE(shadowmap);
	SAFE_RELEASE(sincoeffs);
	SAFE_RELEASE(coscoeffs);
	SAFE_RELEASE(blur);
	SAFE_RELEASE(text);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(texture2);
	SAFE_RELEASE(texture3);

	DXKillAnyRogueObject();
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
}
//*************************************************************************************************************
void Update(float delta)
{
	D3DXVECTOR2 cameravelocity(mousedx, mousedy);

	cameraangle.prev = cameraangle.curr;

	if( mousedown == 1 )
		cameraangle.curr += cameravelocity * 0.004f;

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
void RenderScene(LPD3DXEFFECT effect, int what)
{
	D3DXMATRIX inv;
	D3DXVECTOR4 uv(1, 1, 1, 1);
	D3DXVECTOR4 spec(1, 1, 1, 1);

	for( int i = 0; i < numobjects; ++i )
	{
		SceneObject& obj = objects[i];

		if( obj.behavior == what )
		{
			D3DXMatrixInverse(&inv, 0, &obj.world);

			effect->SetMatrix("matWorld", &obj.world);
			effect->SetMatrix("matWorldInv", &inv);

			if( obj.type == FLOOR )
			{
				uv.x = uv.y = 3;
				spec = D3DXVECTOR4(0.15f, 0.15f, 0.15f, 20.0f);

				effect->SetVector("uv", &uv);
				effect->SetVector("matSpecular", &spec);
				effect->CommitChanges();

				device->SetTexture(0, texture2);
				box->DrawSubset(0);
			}
			else if( obj.type == CRATE )
			{
				uv.x = uv.y = 1;
				spec = D3DXVECTOR4(0.2f, 0.2f, 0.2f, 20.0f);

				effect->SetVector("uv", &uv);
				effect->SetVector("matSpecular", &spec);
				effect->CommitChanges();

				device->SetTexture(0, texture3);
				box->DrawSubset(0);
			}
			else if( obj.type == SKULL )
			{
				uv.x = uv.y = 1;
				spec = D3DXVECTOR4(0.75f, 0.75f, 0.75f, 80.0f);

				effect->SetVector("uv", &uv);
				effect->SetVector("matSpecular", &spec);
				effect->CommitChanges();

				device->SetTexture(0, texture1);
				skull->DrawSubset(0);
			}
		}
	}
}
//*************************************************************************************************************
void BlurTexture(LPDIRECT3DTEXTURE9 tex)
{
	LPDIRECT3DSURFACE9 surface = NULL;
	LPDIRECT3DSURFACE9 blursurface = NULL;

	D3DXVECTOR4 texelsize(1.0f / SHADOWMAP_SIZE, 0, 0, 0);

	blur->GetSurfaceLevel(0, &blursurface);
	tex->GetSurfaceLevel(0, &surface);

	device->SetRenderTarget(0, blursurface);
	device->SetTexture(0, tex);

	boxblur5x5->SetVector("texelSize", &texelsize);

	boxblur5x5->Begin(NULL, 0);
	boxblur5x5->BeginPass(0);
	{
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &vertices[0], 6 * sizeof(float));
		std::swap(texelsize.x, texelsize.y);

		boxblur5x5->SetVector("texelSize", &texelsize);
		boxblur5x5->CommitChanges();

		device->SetRenderTarget(0, surface);
		device->SetTexture(0, blur);
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &vertices[0], 6 * sizeof(float));
	}
	boxblur5x5->EndPass();
	boxblur5x5->End();

	surface->Release();
	blursurface->Release();
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	LPDIRECT3DSURFACE9 oldsurface		= NULL;
	LPDIRECT3DSURFACE9 shadowsurface	= NULL;
	LPDIRECT3DSURFACE9 sinsurface		= NULL;
	LPDIRECT3DSURFACE9 cossurface		= NULL;

	D3DXMATRIX view, proj;
	D3DXMATRIX inv, vp, tmp;
	D3DXMATRIX lightview, lightproj, lightvp;

	D3DXVECTOR2 orient = cameraangle.smooth(alpha);
	D3DXVECTOR4 lightpos(-1, 6, 5, 1);
	D3DXVECTOR3 look(0, 0.5f, 0), up(0, 1, 0);
	D3DXVECTOR3 eye(0, 0, -5.2f);
	D3DXVECTOR4 clipplanes(0.1f, 20, 0, 0);
	D3DXVECTOR4 texelsize(1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE, 0, 0);

	// setup camera
	D3DXMatrixRotationYawPitchRoll(&tmp, orient.x, orient.y, 0);
	D3DXVec3TransformCoord(&eye, &eye, &tmp);

	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)screenwidth / (float)screenheight, clipplanes.x, clipplanes.y);
	D3DXMatrixMultiply(&vp, &view, &proj);

	// setup light
	D3DXMatrixLookAtLH(&lightview, (D3DXVECTOR3*)&lightpos, &look, &up);
	D3DXMatrixPerspectiveFovLH(&lightproj, D3DX_PI / 4, 1, 1, 15.0f); // TODO: fit
	D3DXMatrixMultiply(&lightvp, &lightview, &lightproj);

	if( SUCCEEDED(device->BeginScene()) )
	{
		// STEP 1: render shadow map
		shadowmap->GetSurfaceLevel(0, &shadowsurface);

		device->GetRenderTarget(0, &oldsurface);
		device->SetRenderTarget(0, shadowsurface);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);

		shadowsurface->Release();
		
		convolution->SetTechnique("shadowmap");
		convolution->SetMatrix("lightView", &lightview);
		convolution->SetMatrix("lightProj", &lightproj);
		convolution->SetVector("clipPlanes", &clipplanes);

		convolution->Begin(NULL, 0);
		convolution->BeginPass(0);
		{
			RenderScene(convolution, 1);
			RenderScene(convolution, 3);
		}
		convolution->EndPass();
		convolution->End();

		// STEP 2: evaluate basis functions
		sincoeffs->GetSurfaceLevel(0, &sinsurface);
		coscoeffs->GetSurfaceLevel(0, &cossurface);

		device->SetVertexDeclaration(vertexdecl);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);

		device->SetRenderTarget(0, sinsurface);
		device->SetRenderTarget(1, cossurface);
		device->SetTexture(0, shadowmap);

		sinsurface->Release();
		cossurface->Release();

		convolution->SetTechnique("evalbasis");
		convolution->SetVector("texelSize", &texelsize);

		convolution->Begin(NULL, 0);
		convolution->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &vertices[0], 6 * sizeof(float));
		}
		convolution->EndPass();
		convolution->End();

		device->SetRenderTarget(1, NULL);

		// STEP 3: apply filter
		BlurTexture(sincoeffs);
		BlurTexture(coscoeffs);

		device->SetRenderState(D3DRS_ZENABLE, TRUE);

		// STEP 4: render scene
		device->SetRenderTarget(0, oldsurface);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);

		oldsurface->Release();

		device->SetTexture(1, sincoeffs);
		device->SetTexture(2, coscoeffs);

		convolution->SetTechnique("convolution");
		convolution->SetMatrix("matViewProj", &vp);
		convolution->SetMatrix("lightView", &lightview);
		convolution->SetMatrix("lightProj", &lightproj);
		convolution->SetVector("clipPlanes", &clipplanes);
		convolution->SetVector("lightPos", &lightpos);
		convolution->SetVector("eyePos", (D3DXVECTOR4*)&eye);

		convolution->Begin(NULL, 0);
		convolution->BeginPass(0);
		{
			RenderScene(convolution, 2);
			RenderScene(convolution, 3);
		}
		convolution->EndPass();
		convolution->End();

		device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

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
