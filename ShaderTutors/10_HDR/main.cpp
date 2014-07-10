//*************************************************************************************************************
#include <iostream>
#include <string>

#include "../common/dxext.h"

#define HELP_TEXT	\
	"Hold left button to rotate camera\nor right button to rotate object\n\n" \
	"H - Toggle help text\n" \
	"0 - change object\n" \
	"1 - toggle afterimage\n" \
	"+/- adjust target luminance\n\n" \
	"Target luminance is: "

// helper macros
#define TITLE				"Shader tutorial 10: HDR effects"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern long		screenwidth;
extern long		screenheight;
extern short	mousedx;
extern short	mousedy;
extern short	mousedown;

extern LPDIRECT3D9 direct3d;
extern LPDIRECT3DDEVICE9 device;
extern HWND hwnd;

// tutorial variables
LPD3DXMESH						mesh				= NULL;
LPD3DXMESH						mesh1				= NULL;
LPD3DXMESH						mesh2				= NULL;
LPD3DXMESH						mesh3				= NULL;
LPD3DXMESH						skymesh				= NULL;
LPD3DXEFFECT					fresnel				= NULL;
LPD3DXEFFECT					skyeffect			= NULL;
LPD3DXEFFECT					hdreffect			= NULL;

LPDIRECT3DTEXTURE9				text				= NULL;
LPDIRECT3DTEXTURE9				texture				= NULL;
LPDIRECT3DTEXTURE9				fresneltexture		= NULL;
LPDIRECT3DCUBETEXTURE9			skytexture			= NULL;
LPDIRECT3DCUBETEXTURE9			roughspecular		= NULL;
LPDIRECT3DVERTEXDECLARATION9	vertexdecl			= NULL;

LPDIRECT3DTEXTURE9				scenetarget			= NULL;
LPDIRECT3DTEXTURE9				afterimages[2]		= { 0, 0 };				// ping-pong between them
LPDIRECT3DTEXTURE9				dstargets[5]		= { 0, 0, 0, 0, 0 };	// [0] is bright pass
LPDIRECT3DTEXTURE9				blurtargets[5]		= { 0, 0, 0, 0, 0 };	// [0] is final blur
LPDIRECT3DTEXTURE9				startargets[4][2];
LPDIRECT3DTEXTURE9				ghosttargets[2]		= { 0, 0 };
LPDIRECT3DTEXTURE9				avglumtargets[6]	= { 0, 0, 0, 0, 0, 0 };

LPDIRECT3DSURFACE9				scenesurface		= NULL;
LPDIRECT3DSURFACE9				dssurfaces[5]		= { 0, 0, 0, 0, 0 };
LPDIRECT3DSURFACE9				blursurfaces[5]		= { 0, 0, 0, 0, 0 };
LPDIRECT3DSURFACE9				starsurfaces[4][2];
LPDIRECT3DSURFACE9				ghostsurfaces[2]	= { 0, 0 };
LPDIRECT3DSURFACE9				aftersurfaces[2]	= { 0, 0 };
LPDIRECT3DSURFACE9				avglumsurfaces[6]	= { 0, 0, 0, 0, 0, 0 };

// tutorial variables
state<D3DXVECTOR2>	cameraangle;
state<D3DXVECTOR2>	objectangle;
state<float>		exposure;		// not used during auto exposure

D3DXMATRIX			world, view, proj;
D3DXVECTOR2			cameravelocity;
D3DXVECTOR2			objectvelocity;
D3DXVECTOR2			destcameravelocity;
D3DXVECTOR2			destobjectvelocity;
D3DXVECTOR4			texelsize(0, 0, 0, 1);

float				tmpvert[48];
float				exposurevelocity;
float				destexposurevelocity;
float				targetluminance;
short				adapttex = 4;		// ping-pong between 1x1 textures
short				afterimagetex = 0;
bool				drawhelp = true;
bool				firstframe = true;	// must clear some textures
bool				afterimage = false;

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
	9.5f,	512.0f + 9.5f,	0, 1,	0, 1,

	9.5f,	512.0f + 9.5f,	0, 1,	0, 1,
	521.5f,			9.5f,	0, 1,	1, 0,
	521.5f,	512.0f + 9.5f,	0, 1,	1, 1
};

void UpdateText()
{
	std::string str(256, ' ');
	
	sprintf(&str[0], "%s%f", HELP_TEXT, targetluminance);
	DXRenderText(str.c_str(), text, 512, 512);
}

HRESULT InitScene()
{
	HRESULT hr;
	D3DDISPLAYMODE mode;

	if( FAILED(hr = direct3d->GetAdapterDisplayMode(0, &mode)) )
	{
		MYERROR("Could not get adapter mode");
		return hr;
	}
	
	if( FAILED(hr = direct3d->CheckDeviceFormat( 0, D3DDEVTYPE_HAL, mode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F)) )
	{
		MYERROR("No floating point rendertarget support");
		return hr;
	}
	
	// más depth/stencil-el még müködhet
	if( FAILED(hr = direct3d->CheckDepthStencilMatch( 0, D3DDEVTYPE_HAL, mode.Format, D3DFMT_A16B16G16R16F, D3DFMT_D24S8)) )
	{
		MYERROR("D3DFMT_A16B16G16R16F does not support D3DFMT_D24S8");
		return hr;
	}

	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};
	
	SetWindowText(hwnd, TITLE);

	MYVALID(D3DXLoadMeshFromX("../media/meshes/skullocc3.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh1));
	MYVALID(D3DXLoadMeshFromX("../media/meshes//knot.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh2));
	MYVALID(D3DXLoadMeshFromX("../media/meshes//teapot.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh3));
	MYVALID(D3DXLoadMeshFromX("../media/meshes/sky.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skymesh));

	mesh = mesh1;

	//MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/altar.dds", &skytexture));
	//MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/altar_rough.dds", &roughspecular));
	MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/grace.dds", &skytexture));
	MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/grace_rough.dds", &roughspecular));
	//MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/beach.dds", &skytexture));
	//MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/beach_rough.dds", &roughspecular));
	//MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/stpeters.dds", &skytexture));
	//MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/stpeters_rough.dds", &roughspecular));

	MYVALID(D3DXCreateTextureFromFile(device, "../media/textures/gold.jpg", &texture));
	MYVALID(D3DXCreateTextureFromFile(device, "../media/textures/fresnel.png", &fresneltexture));

	// downsample & blur textures
	for( int i = 0; i < 5; ++i )
	{
		MYVALID(device->CreateTexture(screenwidth / (2 << i), screenheight / (2 << i), 1, D3DUSAGE_RENDERTARGET,
			D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &dstargets[i], NULL));

		MYVALID(device->CreateTexture(screenwidth / (2 << i), screenheight / (2 << i), 1, D3DUSAGE_RENDERTARGET,
			D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &blurtargets[i], NULL));

		MYVALID(blurtargets[i]->GetSurfaceLevel(0, &blursurfaces[i]));
		MYVALID(dstargets[i]->GetSurfaceLevel(0, &dssurfaces[i]));
	}

	// star textures (8x 1 MB @ 1080p)
	for( int i = 0; i < 4; ++i )
	{
		for( int j = 0; j < 2; ++j )
		{
			MYVALID(device->CreateTexture(screenwidth / 4, screenheight / 4, 1, D3DUSAGE_RENDERTARGET,
				D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &startargets[i][j], NULL));

			MYVALID(startargets[i][j]->GetSurfaceLevel(0, &starsurfaces[i][j]));
		}
	}

	// lens flare textures (2x 4 MB @ 1080p)
	for( int i = 0; i < 2; ++i )
	{
		MYVALID(device->CreateTexture(screenwidth / 2, screenheight / 2, 1, D3DUSAGE_RENDERTARGET,
			D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &ghosttargets[i], NULL));

		MYVALID(ghosttargets[i]->GetSurfaceLevel(0, &ghostsurfaces[i]));
	}

	// luminance textures
	for( int i = 0; i < 4; ++i )
	{
		UINT j = 256 / (4 << (2 * i));

		MYVALID(device->CreateTexture(j, j, 1, D3DUSAGE_RENDERTARGET,
			D3DFMT_R16F, D3DPOOL_DEFAULT, &avglumtargets[i], NULL));

		MYVALID(avglumtargets[i]->GetSurfaceLevel(0, &avglumsurfaces[i]));
	}

	// adapted luminance textures
	MYVALID(device->CreateTexture(1, 1, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R16F, D3DPOOL_DEFAULT, &avglumtargets[4], NULL));
	MYVALID(device->CreateTexture(1, 1, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R16F, D3DPOOL_DEFAULT, &avglumtargets[5], NULL));

	MYVALID(avglumtargets[4]->GetSurfaceLevel(0, &avglumsurfaces[4]));
	MYVALID(avglumtargets[5]->GetSurfaceLevel(0, &avglumsurfaces[5]));

	// afterimage textures (2x 4 MB @ 1080p)
	MYVALID(device->CreateTexture(screenwidth / 2, screenheight / 2, 1, D3DUSAGE_RENDERTARGET,
		D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &afterimages[0], NULL));

	MYVALID(device->CreateTexture(screenwidth / 2, screenheight / 2, 1, D3DUSAGE_RENDERTARGET,
		D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &afterimages[1], NULL));

	MYVALID(afterimages[0]->GetSurfaceLevel(0, &aftersurfaces[0]));
	MYVALID(afterimages[1]->GetSurfaceLevel(0, &aftersurfaces[1]));

	// other
	MYVALID(device->CreateTexture(512, 512, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &scenetarget, NULL));
	MYVALID(device->CreateVertexDeclaration(elem, &vertexdecl));

	// other
	MYVALID(scenetarget->GetSurfaceLevel(0, &scenesurface));

	MYVALID(DXCreateEffect("../media/shaders/hdreffects.fx", device, &hdreffect));
	MYVALID(DXCreateEffect("../media/shaders/hdrfresnel.fx", device, &fresnel));
	MYVALID(DXCreateEffect("../media/shaders/sky.fx", device, &skyeffect));

	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

	device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
	device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);

	device->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	device->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
	device->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);

	device->SetSamplerState(3, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(3, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(3, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	device->SetSamplerState(4, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(4, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(4, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	device->SetSamplerState(5, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	device->SetSamplerState(5, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	device->SetSamplerState(5, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	DXRenderText(HELP_TEXT, text, 512, 512);

	// setup camera
	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)screenwidth / (float)screenheight, 1, 50);
	D3DXMatrixIdentity(&world);

	cameraangle				= D3DXVECTOR2(0.6f, 0.1f);
	objectangle				= D3DXVECTOR2(0, 0);
	exposurevelocity		= 0;
	destexposurevelocity	= 0;
	exposure				= 0.05f;
	targetluminance			= 0.03f;

	UpdateText();

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_RELEASE(skyeffect);
	SAFE_RELEASE(hdreffect);
	SAFE_RELEASE(fresnel);
	SAFE_RELEASE(skymesh);
	SAFE_RELEASE(mesh1);
	SAFE_RELEASE(mesh2);
	SAFE_RELEASE(mesh3);

	SAFE_RELEASE(text);
	SAFE_RELEASE(texture);
	SAFE_RELEASE(fresneltexture);
	SAFE_RELEASE(skytexture);
	SAFE_RELEASE(roughspecular);
	SAFE_RELEASE(vertexdecl);

	SAFE_RELEASE(scenesurface);
	SAFE_RELEASE(scenetarget);

	SAFE_RELEASE(ghostsurfaces[0]);
	SAFE_RELEASE(ghostsurfaces[1]);
	SAFE_RELEASE(ghosttargets[0]);
	SAFE_RELEASE(ghosttargets[1]);

	SAFE_RELEASE(aftersurfaces[0]);
	SAFE_RELEASE(aftersurfaces[1]);
	SAFE_RELEASE(afterimages[0]);
	SAFE_RELEASE(afterimages[1]);

	for( int i = 0; i < 6; ++i )
	{
		SAFE_RELEASE(avglumsurfaces[i]);
		SAFE_RELEASE(avglumtargets[i]);
	}

	for( int i = 0; i < 5; ++i )
	{
		SAFE_RELEASE(dssurfaces[i]);
		SAFE_RELEASE(blursurfaces[i]);

		SAFE_RELEASE(dstargets[i]);
		SAFE_RELEASE(blurtargets[i]);
	}

	for( int i = 0; i < 4; ++i )
	{
		for( int j = 0; j < 2; ++j )
		{
			SAFE_RELEASE(starsurfaces[i][j]);
			SAFE_RELEASE(startargets[i][j]);
		}
	}

	DXKillAnyRogueObject();
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	switch( wparam )
	{
	case 0x30:
		if( mesh == mesh1 )
			mesh = mesh2;
		else if( mesh == mesh2 )
			mesh = mesh3;
		else
			mesh = mesh1;

		break;

	case 0x31:
		afterimage = !afterimage;
		break;

	case 0x48:
		drawhelp = !drawhelp;
		break;

	case VK_ADD:
		targetluminance += 0.001f;
		UpdateText();
		break;

	case VK_SUBTRACT:
		targetluminance = max(0.005f, targetluminance - 0.001f);
		UpdateText();
		break;

	default:
		break;
	}
}
//*************************************************************************************************************
void Update(float delta)
{
	// update exposure
	destexposurevelocity = 0;
	exposurevelocity = exposurevelocity + 0.4f * (destexposurevelocity - exposurevelocity);

	exposure.prev = exposure.curr;

	exposure.curr += exposurevelocity;
	exposure.curr = min(16.0f, max(0.004f, exposure.curr));

	// update cmaera & object
	D3DXVECTOR2 dir((float)mousedx, (float)mousedy);

	destcameravelocity = D3DXVECTOR2(0, 0);
	destobjectvelocity = D3DXVECTOR2(0, 0);

	if( mousedown == 1 )
		destcameravelocity = dir * 0.01f;

	if( mousedown == 2 )
		destobjectvelocity = dir * 0.01f;

	if( D3DXVec2Length(&destcameravelocity) > 0.7f )
	{
		D3DXVec2Normalize(&destcameravelocity, &destcameravelocity);
		destcameravelocity *= 0.7f;
	}

	if( D3DXVec2Length(&destobjectvelocity) > 0.7f )
	{
		D3DXVec2Normalize(&destobjectvelocity, &destobjectvelocity);
		destobjectvelocity *= 0.7f;
	}

	cameravelocity = cameravelocity + 0.3f * (destcameravelocity - cameravelocity);
	objectvelocity = objectvelocity + 0.3f * (destobjectvelocity - objectvelocity);

	cameraangle.prev = cameraangle.curr;
	cameraangle.curr += cameravelocity;

	objectangle.prev = objectangle.curr;
	objectangle.curr -= objectvelocity;

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
void MeasureLuminance()
{
	// measure luminance into 64x64 texture
	device->SetRenderTarget(0, avglumsurfaces[0]);
	device->SetTexture(0, scenetarget);

	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	tmpvert[6] = tmpvert[24] = tmpvert[30]		= (float)64.0f - 0.5f;
	tmpvert[13] = tmpvert[19] = tmpvert[31]		= (float)64.0f - 0.5f;

	texelsize.x = 1.0f / (float)screenwidth;
	texelsize.y = 1.0f / (float)screenheight;

	hdreffect->SetTechnique("avglum");
	hdreffect->SetVector("texelsize", &texelsize);

	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);
	{
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
	}
	hdreffect->EndPass();
	hdreffect->End();

	// sample luminance texture down to 4x4
	texelsize.x = 1.0f / 64.0f;
	texelsize.y = 1.0f / 64.0f;

	hdreffect->SetTechnique("avglumdownsample");
	hdreffect->SetVector("texelsize", &texelsize);

	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);

	for( int i = 1; i < 3; ++i )
	{
		device->SetRenderTarget(0, avglumsurfaces[i]);
		device->SetTexture(0, avglumtargets[i - 1]);

		tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)(256 / (4 << (2 * i))) - 0.5f;
		tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)(256 / (4 << (2 * i))) - 0.5f;

		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));

		texelsize.x *= 4;
		texelsize.y *= 4;

		hdreffect->SetVector("texelsize", &texelsize);
		hdreffect->CommitChanges();
	}
	hdreffect->EndPass();
	hdreffect->End();

	// calculate final luminance from 4x4 to 1x1 (apply exp function)
	device->SetRenderTarget(0, avglumsurfaces[3]);
	device->SetTexture(0, avglumtargets[2]);

	texelsize.x = 1.0f / 4.0f;
	texelsize.y = 1.0f / 4.0f;

	hdreffect->SetTechnique("avglumfinal");
	hdreffect->SetVector("texelsize", &texelsize);

	tmpvert[6] = tmpvert[24] = tmpvert[30] = 0.5f;
	tmpvert[13] = tmpvert[19] = tmpvert[31] = 0.5f;

	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);
	{
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
	}
	hdreffect->EndPass();
	hdreffect->End();
}
//*************************************************************************************************************
void AdaptLuminance(float dt)
{
	char othertex = 4 + (5 - adapttex);

	// adapt luminance with time into other 1x1 texture (ping-pong them)
	device->SetRenderTarget(0, avglumsurfaces[othertex]);
	device->SetTexture(0, avglumtargets[adapttex]);	// old adapted luminance
	device->SetTexture(1, avglumtargets[3]);		// result of MeasureLuminance()

	hdreffect->SetTechnique("adaptluminance");
	hdreffect->SetFloat("elapsedtime", dt);

	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);
	{
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
	}
	hdreffect->EndPass();
	hdreffect->End();

	device->SetTexture(5, avglumtargets[othertex]);
	adapttex = othertex;
}
//*************************************************************************************************************
void BrightPass()
{
	device->SetRenderTarget(0, dssurfaces[0]);
	device->SetTexture(0, scenetarget);

	tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth * 0.5f - 0.5f;
	tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight * 0.5f - 0.5f;

	hdreffect->SetTechnique("brightpass");

	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);
	{
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
	}
	hdreffect->EndPass();
	hdreffect->End();

	// generate afterimage now
	device->SetRenderTarget(0, aftersurfaces[afterimagetex]);

	if( afterimage )
	{
		device->SetTexture(0, afterimages[1 - afterimagetex]);
		device->SetTexture(1, dstargets[0]);

		hdreffect->SetTechnique("afterimage");

		hdreffect->Begin(NULL, 0);
		hdreffect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		hdreffect->EndPass();
		hdreffect->End();
	}
	else
		device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1, 0);

	afterimagetex = 1 - afterimagetex;
}
//*************************************************************************************************************
void DownSample()
{
	hdreffect->SetTechnique("downsample");
	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);

	for( int i = 1; i < 5; ++i )
	{
		device->SetRenderTarget(0, dssurfaces[i]);
		device->SetTexture(0, dstargets[i - 1]);

		texelsize.x = (float)(2 << i) / (float)screenwidth;
		texelsize.y = (float)(2 << i) / (float)screenheight;

		tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth / ((float)(2 << i)) - 0.5f;
		tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight / ((float)(2 << i)) - 0.5f;

		hdreffect->SetVector("texelsize", &texelsize);
		hdreffect->CommitChanges();

		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
	}

	hdreffect->EndPass();
	hdreffect->End();
}
//*************************************************************************************************************
void Blur()
{
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);

	// x
	hdreffect->SetTechnique("blurx");
	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);

	for( int i = 0; i < 5; ++i )
	{
		device->SetRenderTarget(0, blursurfaces[i]);
		device->SetTexture(0, dstargets[i]);

		texelsize.x = (float)(2 << i) / (float)screenwidth;
		texelsize.y = (float)(2 << i) / (float)screenheight;

		tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth / ((float)(2 << i)) - 0.5f;
		tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight / ((float)(2 << i)) - 0.5f;

		hdreffect->SetVector("texelsize", &texelsize);
		hdreffect->CommitChanges();

		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
	}

	hdreffect->EndPass();
	hdreffect->End();

	// y
	hdreffect->SetTechnique("blury");
	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);

	for( int i = 0; i < 5; ++i )
	{
		device->SetRenderTarget(0, dssurfaces[i]);
		device->SetTexture(0, blurtargets[i]);

		texelsize.x = (float)(2 << i) / (float)screenwidth;
		texelsize.y = (float)(2 << i) / (float)screenheight;

		tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth / ((float)(2 << i)) - 0.5f;
		tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight / ((float)(2 << i)) - 0.5f;

		hdreffect->SetVector("texelsize", &texelsize);
		hdreffect->CommitChanges();

		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
	}

	hdreffect->EndPass();
	hdreffect->End();

	// combine
	device->SetRenderTarget(0, blursurfaces[0]);
	device->SetTexture(0, dstargets[0]);
	device->SetTexture(1, dstargets[1]);
	device->SetTexture(2, dstargets[2]);
	device->SetTexture(3, dstargets[3]);
	device->SetTexture(4, dstargets[4]);

	hdreffect->SetTechnique("blurcombine");

	tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth * 0.5f - 0.5f;
	tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight * 0.5f - 0.5f;

	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);
	{
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
	}
	hdreffect->EndPass();
	hdreffect->End();
}
//*************************************************************************************************************
void LensFlare()
{
	texelsize.x = 2.0f / (float)screenwidth;
	texelsize.y = 2.0f / (float)screenheight;

	hdreffect->SetTechnique("ghost");
	hdreffect->SetVector("texelsize", &texelsize);

	device->SetRenderTarget(0, ghostsurfaces[0]);
	device->SetTexture(0, dstargets[0]);
	device->SetTexture(1, dstargets[1]);
	device->SetTexture(2, dstargets[2]);

	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);

	tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth / 2.0f - 0.5f;
	tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight / 2.0f - 0.5f;

	// 1 iteration only (not too spectatular...)
	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);
	{
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
	}
	hdreffect->EndPass();
	hdreffect->End();
}
//*************************************************************************************************************
void Star()
{
	tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth / 4.0f - 0.5f;
	tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight / 4.0f - 0.5f;

	texelsize.x = 4.0f / (float)screenwidth;
	texelsize.y = 4.0f / (float)screenheight;

	hdreffect->SetTechnique("star");
	hdreffect->SetVector("texelsize", &texelsize);

	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);

	for( int i = 0; i < 4; ++i )
	{
		hdreffect->SetInt("stardir", i);

		for( int j = 0; j < 3; ++j )
		{
			int ind = (j % 2);

			device->SetRenderTarget(0, starsurfaces[i][ind]);
			device->SetTexture(0, (j == 0 ? dstargets[1] : startargets[i][1 - ind]));

			hdreffect->SetInt("starpass", j);
			hdreffect->CommitChanges();

			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
	}

	hdreffect->EndPass();
	hdreffect->End();

	// combine star textures
	hdreffect->SetTechnique("starcombine");

	device->SetRenderTarget(0, blursurfaces[1]);
	device->SetTexture(0, startargets[0][0]);
	device->SetTexture(1, startargets[1][0]);
	device->SetTexture(2, startargets[2][0]);
	device->SetTexture(3, startargets[3][0]);

	hdreffect->Begin(NULL, 0);
	hdreffect->BeginPass(0);
	{
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
	}
	hdreffect->EndPass();
	hdreffect->End();
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	LPDIRECT3DSURFACE9 oldtarget = NULL;

	D3DXMATRIX		vp, inv, tmp1, tmp2;
	D3DXVECTOR3		axis(0, 1, 0);
	D3DXVECTOR3		eye(0, 0, -5);
	D3DXVECTOR3		look(0, 0, 0);
	D3DXVECTOR3		up(0, 1, 0);

	D3DXVECTOR2		cangle	= cameraangle.smooth(alpha);
	D3DXVECTOR2		oangle	= objectangle.smooth(alpha);
	float			expo	= exposure.smooth(alpha);

	D3DXMatrixRotationYawPitchRoll(&world, cangle.x, cangle.y, 0);
	D3DXVec3TransformCoord(&eye, &eye, &world);

	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixMultiply(&vp, &view, &proj);
	D3DXMatrixInverse(&inv, NULL, &view);

	memcpy(&eye, inv.m[3], 3 * sizeof(float));

	if( mesh == mesh1 )
	{
		// skullocc
		D3DXMatrixScaling(&world, 0.4f, 0.4f, 0.4f);
		world._42 = -1.5f;
	}
	else if( mesh == mesh2 )
	{
		// knot
		D3DXMatrixScaling(&world, 0.8f, 0.8f, 0.8f);
	}
	else
	{
		// teapot
		D3DXMatrixScaling(&world, 1.5f, 1.5f, 1.5f);
	}

	D3DXMatrixRotationYawPitchRoll(&tmp1, oangle.x, oangle.y, 0);
	D3DXMatrixMultiply(&world, &world, &tmp1);
	D3DXMatrixInverse(&inv, NULL, &world);

	fresnel->SetVector("eyePos", (D3DXVECTOR4*)&eye);
	fresnel->SetMatrix("matWorld", &world);
	fresnel->SetMatrix("matWorldInv", &inv);
	fresnel->SetMatrix("matViewProj", &vp);

	D3DXMatrixScaling(&world, 20, 20, 20);
	skyeffect->SetMatrix("matWorld", &world);

	D3DXMatrixIdentity(&world);
	skyeffect->SetMatrix("matWorldSky", &world);
	skyeffect->SetMatrix("matViewProj", &vp);

	memcpy(tmpvert, quadvertices, 36 * sizeof(float));

	if( SUCCEEDED(device->BeginScene()) )
	{
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

		// STEP 1: render sky
		device->GetRenderTarget(0, &oldtarget);

		if( firstframe )
		{
			device->SetRenderTarget(0, aftersurfaces[0]);
			device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

			device->SetRenderTarget(0, aftersurfaces[1]);
			device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

			device->SetRenderTarget(0, avglumsurfaces[4]);
			device->Clear(0, NULL, D3DCLEAR_TARGET, 0x11111111, 1.0f, 0);

			device->SetRenderTarget(0, avglumsurfaces[5]);
			device->Clear(0, NULL, D3DCLEAR_TARGET, 0x11111111, 1.0f, 0);

			firstframe = false;
		}

		device->SetRenderTarget(0, scenesurface);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);

		device->SetTexture(0, skytexture);

		skyeffect->Begin(NULL, 0);
		skyeffect->BeginPass(0);
		{
			skymesh->DrawSubset(0);
		}
		skyeffect->EndPass();
		skyeffect->End();
		
		device->SetRenderState(D3DRS_ZENABLE, TRUE);

		// STEP 2: render object
		device->SetTexture(0, texture);
		device->SetTexture(1, fresneltexture);
		device->SetTexture(2, skytexture);
		device->SetTexture(3, roughspecular);

		fresnel->Begin(NULL, 0);
		fresnel->BeginPass(0);
		{
			mesh->DrawSubset(0);
		}
		fresnel->EndPass();
		fresnel->End();

		device->SetVertexDeclaration(vertexdecl);

		// STEP 3: measure average luminance
		MeasureLuminance();

		// STEP 4: adapt luminance to eye
		AdaptLuminance(elapsedtime);

		// STEP 5: bright pass
		BrightPass();

		// STEP 6: downsample bright pass texture
		DownSample();

		// STEP 7: blur downsampled textures
		Blur();

		// STEP 8: ghost
		LensFlare();

		// STEP 9: star
		Star();

		// STEP 10: final combine
		hdreffect->SetTechnique("final");
		hdreffect->SetFloat("targetluminance", targetluminance);

		device->SetRenderTarget(0, oldtarget);
		device->SetTexture(0, scenetarget);			// scene
		device->SetTexture(1, blurtargets[0]);		// blur
		device->SetTexture(2, blurtargets[1]);		// star
		device->SetTexture(3, ghosttargets[0]);		// ghost
		device->SetTexture(4, afterimages[1 - afterimagetex]);

		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, true);

		oldtarget->Release();

		hdreffect->Begin(NULL, 0);
		hdreffect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadvertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		hdreffect->EndPass();
		hdreffect->End();

		if( drawhelp )
		{
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
		}

		// clean up
		device->SetTexture(1, NULL);
		device->SetTexture(2, NULL);
		device->SetTexture(3, NULL);
		device->SetTexture(4, NULL);
		device->SetTexture(5, NULL);

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
