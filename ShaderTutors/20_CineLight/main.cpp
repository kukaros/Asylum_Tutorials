//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>

#include "../common/dxext.h"

// helper macros
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

// tutorial variables
LPD3DXMESH						skymesh			= NULL;
LPD3DXEFFECT					skyeffect		= NULL;
LPD3DXEFFECT					forward			= NULL;
LPD3DXEFFECT					light			= NULL;
LPD3DXEFFECT					gammacorrect	= NULL;
LPD3DXEFFECT					cinelight		= NULL;
LPDIRECT3DVERTEXDECLARATION9	quaddecl		= NULL;

DXObject*						object1			= NULL;
LPDIRECT3DTEXTURE9				texture3		= NULL;
LPDIRECT3DTEXTURE9				texture4		= NULL;
LPDIRECT3DCUBETEXTURE9			skytex			= NULL;
LPDIRECT3DTEXTURE9				text			= NULL;

LPDIRECT3DTEXTURE9				irradiance1		= NULL;
LPDIRECT3DTEXTURE9				blur1			= NULL;
LPDIRECT3DTEXTURE9				blur2			= NULL;

LPDIRECT3DSURFACE9				irradsurf1		= NULL;
LPDIRECT3DSURFACE9				blursurf1		= NULL;
LPDIRECT3DSURFACE9				blursurf2		= NULL;

state<D3DXVECTOR2>				cameraangle;
D3DXMATRIX						world[3];

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

	if( !object1->Load("../media/meshes/bridge/bridge.qm") )
	{
		MYERROR("object1->Load(\"../media/meshes/bridge/bridge.qm\")");
		return E_FAIL;
	}

	MYVALID(D3DXLoadMeshFromXA("../media/meshes/sky.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skymesh));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/meshes/bridge/bridge_normal.tga", &texture4));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/meshes/bridge/bridge_light.tga", &texture3));
	MYVALID(D3DXCreateCubeTextureFromFileA(device, "../media/textures/sky4.dds", &skytex));

	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &irradiance1, NULL));
	MYVALID(device->CreateTexture(screenwidth / 2, screenheight / 2, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &blur1, NULL));
	MYVALID(device->CreateTexture(screenwidth / 2, screenheight / 2, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &blur2, NULL));
	
	MYVALID(DXCreateEffect("../media/shaders/cinelight.fx", device, &cinelight));
	MYVALID(DXCreateEffect("../media/shaders/gammacorrect.fx", device, &gammacorrect));
	MYVALID(DXCreateEffect("../media/shaders/forward.fx", device, &forward));
	MYVALID(DXCreateEffect("../media/shaders/ambient.fx", device, &light));
	MYVALID(DXCreateEffect("../media/shaders/sky.fx", device, &skyeffect));

	forward->SetTechnique("forward_point_tbn");

	MYVALID(irradiance1->GetSurfaceLevel(0, &irradsurf1));
	MYVALID(blur1->GetSurfaceLevel(0, &blursurf1));
	MYVALID(blur2->GetSurfaceLevel(0, &blursurf2));

	MYVALID(device->CreateTexture(512, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));
	DXRenderText("Hold left button to rotate camera", text, 512, 128);

	cameraangle = D3DXVECTOR2(0.78f, 0.78f);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	delete object1;
	
	SAFE_RELEASE(irradsurf1);
	SAFE_RELEASE(blursurf1);
	SAFE_RELEASE(blursurf2);
	SAFE_RELEASE(irradiance1);
	SAFE_RELEASE(blur1);
	SAFE_RELEASE(blur2);
	SAFE_RELEASE(skymesh);
	SAFE_RELEASE(skytex);
	SAFE_RELEASE(skyeffect);
	SAFE_RELEASE(forward);
	SAFE_RELEASE(gammacorrect);
	SAFE_RELEASE(cinelight);
	SAFE_RELEASE(light);
	SAFE_RELEASE(texture3);
	SAFE_RELEASE(texture4);
	SAFE_RELEASE(text);
	SAFE_RELEASE(quaddecl);
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
}
//*************************************************************************************************************
void Update(float delta)
{
	D3DXVECTOR2 velocity(mousedx, mousedy);

	cameraangle.prev = cameraangle.curr;

	if( mousedown == 1 )
		cameraangle.curr += velocity * 0.004f;

	// clamp to [-pi, pi]
	if( cameraangle.curr.y >= 1.5f )
		cameraangle.curr.y = 1.5f;

	if( cameraangle.curr.y <= -1.5f )
		cameraangle.curr.y = -1.5f;
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	// camera
	D3DXMATRIX		view, proj, viewproj;
	D3DXMATRIX		skyworld, inv;

	D3DXVECTOR4		lightPos;
	D3DXVECTOR3		eye(0, 0, -2.5f);
	D3DXVECTOR3		look(0, 0.5f, 0);
	D3DXVECTOR3		up(0, 1, 0);
	D3DXVECTOR2		orient	= cameraangle.smooth(alpha);

	D3DXMatrixRotationYawPitchRoll(&view, orient.x, orient.y, 0);
	D3DXVec3TransformCoord(&eye, &eye, &view);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)screenwidth / (float)screenheight, 0.1f, 100);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixMultiply(&viewproj, &view, &proj);

	time += elapsedtime;

	// world matrices
	D3DXVECTOR3 sc(0.008f, 0.008f, 0.008f);
	D3DXQUATERNION rot;

	D3DXMatrixScaling(&world[0], 0.5f, 0.5f, 0.5f);
	D3DXMatrixScaling(&world[1], 1, 1, 1);

	world[0]._41 = 0;
	world[0]._42 = -0.25f;
	world[0]._43 = 0;

	world[1]._42 = -1;

	// render
	if( SUCCEEDED(device->BeginScene()) )
	{
		LPDIRECT3DSURFACE9 oldtarget = NULL;

		device->GetRenderTarget(0, &oldtarget);
		device->SetRenderTarget(0, irradsurf1);

		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);

		// render sky
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		D3DXMatrixScaling(&skyworld, 20, 20, 20);
		skyeffect->SetMatrix("matWorld", &skyworld);

		D3DXMatrixRotationX(&skyworld, D3DXToRadian(-30));
		skyeffect->SetMatrix("matWorldSky", &skyworld);

		skyeffect->SetMatrix("matViewProj", &viewproj);
		skyeffect->SetVector("eyePos", (D3DXVECTOR4*)&eye);
		skyeffect->SetFloat("degamma", 2.2f);

		skyeffect->Begin(0, 0);
		skyeffect->BeginPass(0);
		{
			device->SetTexture(0, skytex);
			skymesh->DrawSubset(0);
		}
		skyeffect->EndPass();
		skyeffect->End();

		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

		// render scene
		D3DXMatrixInverse(&inv, NULL, &world[1]);

		forward->SetMatrix("matViewProj", &viewproj);
		forward->SetMatrix("matWorldInv", &inv);
		forward->SetMatrix("matWorld", &world[1]);
		forward->SetVector("eyePos", (D3DXVECTOR4*)&eye);

		device->SetTexture(1, texture4);

		lightPos = D3DXVECTOR4(-1.2f, 1, 1, 5);
		forward->SetVector("lightPos", &lightPos);

		forward->Begin(0, 0);
		forward->BeginPass(0);
		{
			object1->Draw(DXObject::All, forward);

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			lightPos = D3DXVECTOR4(1.2f, 1, 1, 5);
			forward->SetVector("lightPos", &lightPos);
			forward->CommitChanges();

			object1->Draw(DXObject::All, forward);
		}
		forward->EndPass();
		forward->End();

		// draw high luminance regions
		D3DXVECTOR4 intensity(10, 10, 10, 1);

		light->SetMatrix("matViewProj", &viewproj);
		light->SetMatrix("matWorld", &world[1]);
		light->SetVector("ambient", &intensity);

		device->SetTexture(0, texture3);
		device->SetTexture(1, 0);

		light->Begin(0, 0);
		light->BeginPass(0);
		{
			object1->Draw(DXObject::All & (~DXObject::Material), forward);
		}
		light->EndPass();
		light->End();

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		// bright pass
		quadvertices[6] = quadvertices[24] = quadvertices[30] = (float)(screenwidth / 2) - 0.5f;
		quadvertices[13] = quadvertices[19] = quadvertices[31] = (float)(screenheight / 2) - 0.5f;

		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderTarget(0, blursurf1);
		
		device->SetVertexDeclaration(quaddecl);
		device->SetTexture(0, irradiance1);
		
		cinelight->SetTechnique("brightpass");

		cinelight->Begin(0, 0);
		cinelight->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, 6 * sizeof(float));
		}
		cinelight->EndPass();
		cinelight->End();

		// blur
		float radiuses[] = { 1, 2, 4, 8 };

		for( int i = 0; i < 4; ++i )
		{
			device->SetRenderTarget(0, ((i % 2) ? blursurf1 : blursurf2));
			device->SetTexture(0, ((i % 2) ? blur2 : blur1));

			cinelight->SetTechnique("blur");
			cinelight->SetFloat("blurPass", (float)i);

			cinelight->Begin(0, 0);
			cinelight->BeginPass(0);
			{
				device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, 6 * sizeof(float));
			}
			cinelight->EndPass();
			cinelight->End();
		}

		// add to scene
		quadvertices[6] = quadvertices[24] = quadvertices[30] = (float)screenwidth - 0.5f;
		quadvertices[13] = quadvertices[19] = quadvertices[31] = (float)screenheight - 0.5f;

		device->SetRenderTarget(0, irradsurf1);
		device->SetTexture(0, blur1);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

		cinelight->SetTechnique("combine");

		cinelight->Begin(0, 0);
		cinelight->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, 6 * sizeof(float));
		}
		cinelight->EndPass();
		cinelight->End();

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		// gamma correct
		device->SetRenderTarget(0, oldtarget);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);

		device->SetTexture(0, irradiance1);

		gammacorrect->Begin(0, 0);
		gammacorrect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, 6 * sizeof(float));
		}
		gammacorrect->EndPass();
		gammacorrect->End();

		oldtarget->Release();

		/*
		///////////
		D3DXMatrixScaling(&inv, 0.25f, 0.25f, 0.25f);

		inv._41 = 1.2f;
		inv._42 = 1;
		inv._43 = 1;

		device->SetTransform(D3DTS_VIEW, &view);
		device->SetTransform(D3DTS_PROJECTION, &proj);
		device->SetTransform(D3DTS_WORLD, &inv);
		skymesh->DrawSubset(0);
		//////////
		*/

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
