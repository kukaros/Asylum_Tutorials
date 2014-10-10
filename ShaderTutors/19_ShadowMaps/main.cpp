//*************************************************************************************************************
#include <iostream>
#include <string>

#include "../common/dxext.h"

#define SHADOWMAP_SIZE	512

// object types
#define FLOOR	1
#define SKULL	2
#define CRATE	3

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

// external functions
extern void RenderWithConvolution(const D3DXMATRIX&, const D3DXVECTOR3&, const D3DXMATRIX&, const D3DXMATRIX&, const D3DXVECTOR4&, const D3DXVECTOR4&, const D3DXVECTOR4&);
extern void RenderWithExponential(const D3DXMATRIX&, const D3DXVECTOR3&, const D3DXMATRIX&, const D3DXMATRIX&, const D3DXVECTOR4&, const D3DXVECTOR4&, const D3DXVECTOR4&);
extern void RenderWithPCF(const D3DXMATRIX&, const D3DXVECTOR3&, const D3DXMATRIX&, const D3DXMATRIX&, const D3DXVECTOR4&, const D3DXVECTOR4&, const D3DXVECTOR4&);

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
LPD3DXEFFECT					exponential	= NULL;
LPD3DXEFFECT					boxblur5x5	= NULL;
LPD3DXEFFECT					pcf5x5		= NULL;
LPDIRECT3DVERTEXDECLARATION9	vertexdecl	= NULL;
LPDIRECT3DTEXTURE9				texture1	= NULL;
LPDIRECT3DTEXTURE9				texture2	= NULL;
LPDIRECT3DTEXTURE9				texture3	= NULL;
LPDIRECT3DTEXTURE9				shadowmap	= NULL;
LPDIRECT3DTEXTURE9				sincoeffs	= NULL;
LPDIRECT3DTEXTURE9				coscoeffs	= NULL;
LPDIRECT3DTEXTURE9				blurARGB8	= NULL;
LPDIRECT3DTEXTURE9				blurRG32F	= NULL;
LPDIRECT3DTEXTURE9				text		= NULL;

DXAABox							scenebb;
state<D3DXVECTOR2>				cameraangle;
state<D3DXVECTOR2>				lightangle;
int								shadowtech	= 1;

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
	9.5f,	512.0f + 9.5f,	0, 1,	0, 1,

	9.5f,	512.0f + 9.5f,	0, 1,	0, 1,
	521.5f,			9.5f,	0, 1,	1, 0,
	521.5f,	512.0f + 9.5f,	0, 1,	1, 1
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

	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &shadowmap, NULL));
	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &blurRG32F, NULL));
	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &sincoeffs, NULL));
	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &coscoeffs, NULL));
	MYVALID(device->CreateTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &blurARGB8, NULL));
	
	MYVALID(device->CreateTexture(512, 512, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));
	MYVALID(device->CreateVertexDeclaration(elem, &vertexdecl));

	MYVALID(DXCreateEffect("../media/shaders/exponentialshadow.fx", device, &exponential));
	MYVALID(DXCreateEffect("../media/shaders/convolutionshadow.fx", device, &convolution));
	MYVALID(DXCreateEffect("../media/shaders/boxblur5x5.fx", device, &boxblur5x5));
	MYVALID(DXCreateEffect("../media/shaders/pcfshadow5x5.fx", device, &pcf5x5));

	D3DXVECTOR4 noisesize(16.0f, 16.0f, 0, 1);
	D3DXVECTOR4 texelsize(1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE, 0, 1);

	DXRenderText(
		"Use mouse to rotate camera and light\n\n0 - Unfiltered\n1 - PCF (5x5)\n2 - Irregular PCF\n3 - Variance\n"
		"4 - Convolution\n5 - Exponential\n6 - Exponential variance", text, 512, 512);
	
	cameraangle = D3DXVECTOR2(0.78f, 0.78f);
	lightangle = D3DXVECTOR2(3.2f, 0.75f);

	DXAABox			boxbb;
	DXAABox			skullbb;
	DXAABox			tmpbb;
	D3DXMATRIX		tmp1, tmp2, tmp3;
	D3DXVECTOR3*	vdata;

	box->LockVertexBuffer(D3DLOCK_READONLY, (void**)&vdata);
	D3DXComputeBoundingBox(vdata, box->GetNumVertices(), box->GetNumBytesPerVertex(), &boxbb.Min, &boxbb.Max);
	box->UnlockVertexBuffer();

	skull->LockVertexBuffer(D3DLOCK_READONLY, (void**)&vdata);
	D3DXComputeBoundingBox(vdata, skull->GetNumVertices(), skull->GetNumBytesPerVertex(), &skullbb.Min, &skullbb.Max);
	skull->UnlockVertexBuffer();

	for( int i = 0; i < numobjects; ++i )
	{
		SceneObject& obj = objects[i];

		D3DXMatrixScaling(&tmp1, obj.scale.x, obj.scale.y, obj.scale.z);
		D3DXMatrixRotationYawPitchRoll(&tmp2, obj.angles.x, obj.angles.y, obj.angles.z);
		D3DXMatrixTranslation(&tmp3, obj.position.x, obj.position.y, obj.position.z);

		D3DXMatrixMultiply(&obj.world, &tmp1, &tmp2);
		D3DXMatrixMultiply(&obj.world, &obj.world, &tmp3);

		if( obj.type == SKULL )
			tmpbb = skullbb;
		else
			tmpbb = boxbb;

		tmpbb.TransformAxisAligned(obj.world);

		scenebb.Add(tmpbb.Min);
		scenebb.Add(tmpbb.Max);
	}

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_RELEASE(exponential);
	SAFE_RELEASE(convolution);
	SAFE_RELEASE(boxblur5x5);
	SAFE_RELEASE(pcf5x5);
	SAFE_RELEASE(vertexdecl);
	SAFE_RELEASE(skull);
	SAFE_RELEASE(box);
	SAFE_RELEASE(shadowmap);
	SAFE_RELEASE(sincoeffs);
	SAFE_RELEASE(coscoeffs);
	SAFE_RELEASE(blurARGB8);
	SAFE_RELEASE(blurRG32F);
	SAFE_RELEASE(text);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(texture2);
	SAFE_RELEASE(texture3);

	DXKillAnyRogueObject();
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	if( wparam >= 0x30 && wparam <= 0x39 )
	{
		if( (wparam - 0x30) > 0 && (wparam - 0x30) < 7 )
			shadowtech = (wparam - 0x30);
	}
}
//*************************************************************************************************************
void Update(float delta)
{
	D3DXVECTOR2 cameravelocity(mousedx, mousedy);

	cameraangle.prev = cameraangle.curr;
	lightangle.prev = lightangle.curr;

	if( mousedown == 1 )
		cameraangle.curr += cameravelocity * 0.004f;

	if( mousedown == 2 )
		lightangle.curr += cameravelocity * 0.004f;

	// clamp to [-pi, pi]
	if( cameraangle.curr.y >= 1.5f )
		cameraangle.curr.y = 1.5f;

	if( cameraangle.curr.y <= -1.5f )
		cameraangle.curr.y = -1.5f;

	if( lightangle.curr.y >= 1.5f )
		lightangle.curr.y = 1.5f;

	if( lightangle.curr.y <= 0.1f )
		lightangle.curr.y = 0.1f;
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
				spec = D3DXVECTOR4(0.2f, 0.2f, 0.2f, 20.0f);

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
	LPDIRECT3DTEXTURE9 blurtex = NULL;

	D3DXVECTOR4 texelsize(1.0f / SHADOWMAP_SIZE, 0, 0, 0);
	D3DSURFACE_DESC desc;

	tex->GetLevelDesc(0, &desc);

	if( desc.Format == D3DFMT_A8R8G8B8 )
		blurtex = blurARGB8;
	else
		blurtex = blurRG32F;

	blurtex->GetSurfaceLevel(0, &blursurface);
	tex->GetSurfaceLevel(0, &surface);

	device->SetRenderTarget(0, blursurface);
	device->SetTexture(0, tex);
	device->SetVertexDeclaration(vertexdecl);

	boxblur5x5->SetVector("texelSize", &texelsize);

	boxblur5x5->Begin(NULL, 0);
	boxblur5x5->BeginPass(0);
	{
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &vertices[0], 6 * sizeof(float));
		std::swap(texelsize.x, texelsize.y);

		boxblur5x5->SetVector("texelSize", &texelsize);
		boxblur5x5->CommitChanges();

		device->SetRenderTarget(0, surface);
		device->SetTexture(0, blurtex);
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
	D3DXMATRIX view, proj;
	D3DXMATRIX inv, vp, tmp;
	D3DXMATRIX lightview, lightproj, lightvp;

	D3DXVECTOR2 orient = cameraangle.smooth(alpha);
	D3DXVECTOR4 lightpos(0, 0, -5, 0);
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
	orient = lightangle.smooth(alpha);
	look = D3DXVECTOR3(0, 0, 0);

	D3DXMatrixRotationYawPitchRoll(&tmp, orient.x, orient.y, 0);
	D3DXVec4Transform(&lightpos, &lightpos, &tmp);

	D3DXMatrixLookAtLH(&lightview, (D3DXVECTOR3*)&lightpos, &look, &up);
	DXFitToBox(lightproj, lightview, scenebb);
	D3DXMatrixMultiply(&lightvp, &lightview, &lightproj);

	if( SUCCEEDED(device->BeginScene()) )
	{
		switch( shadowtech )
		{
		case 1:
			RenderWithPCF(vp, eye, lightview, lightproj, lightpos, clipplanes, texelsize);
			break;

		case 4:
			RenderWithConvolution(vp, eye, lightview, lightproj, lightpos, clipplanes, texelsize);
			break;

		case 5:
			RenderWithExponential(vp, eye, lightview, lightproj, lightpos, clipplanes, texelsize);
			break;

		default:
			RenderWithPCF(vp, eye, lightview, lightproj, lightpos, clipplanes, texelsize);
			break;
		}

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
