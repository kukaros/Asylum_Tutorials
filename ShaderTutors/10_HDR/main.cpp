//*************************************************************************************************************
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")

#include <d3dx9.h>
#include <mmsystem.h>
#include <iostream>

// helper macros
#define MYERROR(x)		{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)		{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }

// external variables
extern long screenwidth;
extern long screenheight;

extern LPDIRECT3DDEVICE9			device;
extern D3DPRESENT_PARAMETERS		d3dpp;
extern LPDIRECT3DDEVICE9			device;
extern LPD3DXMESH					mesh;
extern LPD3DXMESH					skymesh;
extern LPD3DXEFFECT					effect;
extern LPD3DXEFFECT					skyeffect;
extern LPD3DXEFFECT					bloomeffect;
extern LPDIRECT3DTEXTURE9			texture;
extern LPDIRECT3DTEXTURE9			fresneltexture;
extern LPDIRECT3DCUBETEXTURE9		skytexture;
extern LPDIRECT3DCUBETEXTURE9		roughspecular;
extern LPDIRECT3DVERTEXDECLARATION9	vertexdecl;

extern LPDIRECT3DTEXTURE9			scenetarget;
extern LPDIRECT3DTEXTURE9			dstargets[5];
extern LPDIRECT3DTEXTURE9			blurtargets[5];
extern LPDIRECT3DTEXTURE9			startargets[4][2];
extern LPDIRECT3DTEXTURE9			ghosttargets[2];
extern LPDIRECT3DTEXTURE9			avglumtargets[6];

extern LPDIRECT3DSURFACE9			scenesurface;
extern LPDIRECT3DSURFACE9			dssurfaces[5];
extern LPDIRECT3DSURFACE9			blursurfaces[5];
extern LPDIRECT3DSURFACE9			starsurfaces[4][2];
extern LPDIRECT3DSURFACE9			ghostsurfaces[2];
extern LPDIRECT3DSURFACE9			avglumsurfaces[6];

extern D3DFORMAT					bloomformat;
extern D3DFORMAT					lumformat;
extern short						mousex, mousedx;
extern short						mousey, mousedy;
extern short						mousedown;
extern short						expdir;

// external functions
HRESULT DXCreateEffect(const char* file, LPD3DXEFFECT* out);

// tutorial variables
D3DXMATRIX view, world, proj, wvp;
D3DXVECTOR2 cameravelocity;
D3DXVECTOR2 objectvelocity;
D3DXVECTOR2 destcameravelocity;
D3DXVECTOR2 destobjectvelocity;

float exposurevelocity;
float destexposurevelocity;
short adapttex = 4;

float vertices[36] =
{
	-0.5f, -0.5f, 0, 1, 0, 0,
	(float)screenwidth - 0.5f, -0.5f, 0, 1, 1, 0,
	-0.5f, (float)screenheight - 0.5f, 0, 1, 0, 1,

	-0.5f, (float)screenheight - 0.5f, 0, 1, 0, 1,
	(float)screenwidth - 0.5f, -0.5f, 0, 1, 1, 0,
	(float)screenwidth - 0.5f, (float)screenheight - 0.5f, 0, 1, 1, 1
};

// camera & object state
template <typename T>
struct state
{
	T prev;
	T curr;

	state& operator =(const T& t) {
		prev = curr = t;
		return *this;
	}

	T smooth(float alpha) {
		return prev + alpha * (curr - prev);
	}
};

state<D3DXVECTOR2> cameraangle;
state<D3DXVECTOR2> objectangle;
state<float> exposure;				// not used when auto exposure

HRESULT InitScene()
{
	HRESULT hr;

	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};
	
	//MYVALID(D3DXLoadMeshFromX("../../design/media2/knot2.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh));
	MYVALID(D3DXLoadMeshFromX("../media/meshes/skullocc3.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh));
	MYVALID(D3DXLoadMeshFromX("../media/meshes/sky.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skymesh));

	//MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/altar.dds", &skytexture));
	//MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/altar_rough.dds", &roughspecular));
	MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/grace.dds", &skytexture));
	MYVALID(D3DXCreateCubeTextureFromFile(device, "../media/textures/grace_rough.dds", &roughspecular));

	MYVALID(D3DXCreateTextureFromFile(device, "../media/textures/gold.jpg", &texture));
	MYVALID(D3DXCreateTextureFromFile(device, "../media/textures/fresnel.png", &fresneltexture));

	// downsample & blur textures
	for( int i = 0; i < 5; ++i )
	{
		MYVALID(device->CreateTexture(screenwidth / (2 << i), screenheight / (2 << i), 1, D3DUSAGE_RENDERTARGET,
			bloomformat, D3DPOOL_DEFAULT, &dstargets[i], NULL));

		MYVALID(device->CreateTexture(screenwidth / (2 << i), screenheight / (2 << i), 1, D3DUSAGE_RENDERTARGET,
			bloomformat, D3DPOOL_DEFAULT, &blurtargets[i], NULL));

		MYVALID(blurtargets[i]->GetSurfaceLevel(0, &blursurfaces[i]));
		MYVALID(dstargets[i]->GetSurfaceLevel(0, &dssurfaces[i]));
	}

	// star textures
	for( int i = 0; i < 4; ++i )
	{
		for( int j = 0; j < 2; ++j )
		{
			MYVALID(device->CreateTexture(screenwidth / 4, screenheight / 4, 1, D3DUSAGE_RENDERTARGET,
				bloomformat, D3DPOOL_DEFAULT, &startargets[i][j], NULL));

			MYVALID(startargets[i][j]->GetSurfaceLevel(0, &starsurfaces[i][j]));
		}
	}

	// lens flare textures
	for( int i = 0; i < 2; ++i )
	{
		MYVALID(device->CreateTexture(screenwidth / 2, screenheight / 2, 1, D3DUSAGE_RENDERTARGET,
			bloomformat, D3DPOOL_DEFAULT, &ghosttargets[i], NULL));

		MYVALID(ghosttargets[i]->GetSurfaceLevel(0, &ghostsurfaces[i]));
	}

	// luminance textures
	for( int i = 0; i < 4; ++i )
	{
		UINT j = 256 / (4 << (2 * i));

		MYVALID(device->CreateTexture(j, j, 1, D3DUSAGE_RENDERTARGET,
			lumformat, D3DPOOL_DEFAULT, &avglumtargets[i], NULL));

		MYVALID(avglumtargets[i]->GetSurfaceLevel(0, &avglumsurfaces[i]));
	}

	// adapted luminance textures
	MYVALID(device->CreateTexture(1, 1, 1, D3DUSAGE_RENDERTARGET, lumformat, D3DPOOL_DEFAULT, &avglumtargets[4], NULL));
	MYVALID(device->CreateTexture(1, 1, 1, D3DUSAGE_RENDERTARGET, lumformat, D3DPOOL_DEFAULT, &avglumtargets[5], NULL));

	MYVALID(avglumtargets[4]->GetSurfaceLevel(0, &avglumsurfaces[4]));
	MYVALID(avglumtargets[5]->GetSurfaceLevel(0, &avglumsurfaces[5]));

	// other
	MYVALID(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, bloomformat, D3DPOOL_DEFAULT, &scenetarget, NULL));
	MYVALID(device->CreateVertexDeclaration(elem, &vertexdecl));

	MYVALID(scenetarget->GetSurfaceLevel(0, &scenesurface));

	MYVALID(DXCreateEffect("../media/shaders/fresnel.fx", &effect));
	MYVALID(DXCreateEffect("../media/shaders/sky.fx", &skyeffect));
	MYVALID(DXCreateEffect("../media/shaders/hdr.fx", &bloomeffect));

	// setup camera
	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)d3dpp.BackBufferWidth / (float)d3dpp.BackBufferHeight, 1, 5000);
	D3DXMatrixIdentity(&world);

	cameraangle = D3DXVECTOR2(0.6f, 0.1f);
	exposurevelocity = 0;
	destexposurevelocity = 0;
	exposure = 0.05f;

	return S_OK;
}
//*************************************************************************************************************
void Update(float delta)
{
	// update exposure
	destexposurevelocity = 0;

	if( expdir == 2 )
		destexposurevelocity = 0.1f;
	else if( expdir == 1 )
		destexposurevelocity = -0.1f;

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
void Render(float alpha, float elapsedtime)
{
	unsigned long flags = D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER;
	D3DXMATRIX inv, tmp1, tmp2;

	D3DXVECTOR3 axis(0, 1, 0);
	D3DXVECTOR3 eye(0, 0, -5.0f);
	D3DXVECTOR3 look(0, 0, 0);
	D3DXVECTOR3 up(0, 1, 0);
	D3DXVECTOR4 texelsize(0, 0, 0, 1);
	
	// not used when auto exposure
	float expo = exposure.smooth(alpha);

	D3DXVECTOR2 cangle = cameraangle.smooth(alpha);
	D3DXVECTOR2 oangle = objectangle.smooth(alpha);

	D3DXMatrixRotationYawPitchRoll(&world, cangle.x, cangle.y, 0);
	D3DXVec3TransformCoord(&eye, &eye, &world);

	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixInverse(&inv, NULL, &view);
	D3DXMatrixRotationYawPitchRoll(&tmp1, oangle.x, oangle.y, 0);

	effect->SetVector("eyePos", (D3DXVECTOR4*)inv.m[3]);

	// skullocc
	D3DXMatrixScaling(&world, 0.4f, 0.4f, 0.4f);
	world._42 = -1.5f;
	
	// knot
	//D3DXMatrixScaling(&world, 0.8f, 0.8f, 0.8f);

	// knot2
	//D3DXMatrixScaling(&world, 5.0f, 5.0f, 5.0f);

	D3DXMatrixMultiply(&world, &world, &tmp1);
	D3DXMatrixMultiply(&wvp, &view, &proj);
	D3DXMatrixInverse(&inv, NULL, &world);

	effect->SetMatrix("matWorld", &world);
	effect->SetMatrix("matWorldInv", &inv);
	effect->SetMatrix("matViewProj", &wvp);

	D3DXMatrixScaling(&world, 4407, 4407, 4407);
	skyeffect->SetMatrix("matWorld", &world);

	D3DXMatrixIdentity(&world);
	skyeffect->SetMatrix("matWorldSky", &world);
	skyeffect->SetMatrix("matViewProj", &wvp);

	LPDIRECT3DSURFACE9 oldtarget = NULL;
	float tmpvert[48];

	memcpy(tmpvert, vertices, 36 * sizeof(float));

	if( SUCCEEDED(device->BeginScene()) )
	{
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

		// STEP 1: render scene
		device->GetRenderTarget(0, &oldtarget);
		device->SetRenderTarget(0, scenesurface);
		device->Clear(0, NULL, flags, 0xff6694ed, 1.0f, 0);
		
		device->SetTexture(0, skytexture);

		skyeffect->Begin(NULL, 0);
		skyeffect->BeginPass(0);
		{
			skymesh->DrawSubset(0);
		}
		skyeffect->EndPass();
		skyeffect->End();
		
		device->SetTexture(0, texture);
		device->SetTexture(1, fresneltexture);
		device->SetTexture(2, skytexture);
		device->SetTexture(3, roughspecular);

		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			mesh->DrawSubset(0);
		}
		effect->EndPass();
		effect->End();

		// STEP 2: measure average luminance
		device->SetVertexDeclaration(vertexdecl);
		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)64.0f - 0.5f;
		tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)64.0f - 0.5f;

		texelsize.x = 1.0f / (float)screenwidth;
		texelsize.y = 1.0f / (float)screenheight;

		bloomeffect->SetTechnique("avglum");
		bloomeffect->SetVector("texelsize", &texelsize);

		device->SetTexture(0, scenetarget);
		device->SetRenderTarget(0, avglumsurfaces[0]);

		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		bloomeffect->EndPass();
		bloomeffect->End();

		// downsample luminance texture
		texelsize.x = 1.0f / 64.0f;
		texelsize.y = 1.0f / 64.0f;

		bloomeffect->SetTechnique("avglumdownsample");
		bloomeffect->SetVector("texelsize", &texelsize);

		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);

		for( int i = 1; i < 3; ++i )
		{
			tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)(256 / (4 << (2 * i))) - 0.5f;
			tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)(256 / (4 << (2 * i))) - 0.5f;

			device->SetTexture(0, avglumtargets[i - 1]);
			device->SetRenderTarget(0, avglumsurfaces[i]);
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));

			texelsize.x *= 4;
			texelsize.y *= 4;

			bloomeffect->SetVector("texelsize", &texelsize);
			bloomeffect->CommitChanges();
		}
		bloomeffect->EndPass();
		bloomeffect->End();

		// final average luminance value
		texelsize.x = 1.0f / 4.0f;
		texelsize.y = 1.0f / 4.0f;

		bloomeffect->SetTechnique("avglumfinal");
		bloomeffect->SetVector("texelsize", &texelsize);

		tmpvert[6] = tmpvert[24] = tmpvert[30] = 1.0f - 0.5f;
		tmpvert[13] = tmpvert[19] = tmpvert[31] = 1.0f - 0.5f;

		device->SetTexture(0, avglumtargets[2]);
		device->SetRenderTarget(0, avglumsurfaces[3]);

		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		bloomeffect->EndPass();
		bloomeffect->End();

		// STEP 3: adapt luminance to eye
		bloomeffect->SetTechnique("adaptluminance");
		bloomeffect->SetFloat("elapsedtime", elapsedtime);

		char othertex = 4 + (5 - adapttex);

		device->SetTexture(0, avglumtargets[adapttex]);
		device->SetTexture(1, avglumtargets[3]);
		device->SetRenderTarget(0, avglumsurfaces[othertex]);

		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		bloomeffect->EndPass();
		bloomeffect->End();

		device->SetTexture(5, avglumtargets[othertex]);
		adapttex = othertex;

		// STEP 4: bright pass
		tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth * 0.5f - 0.5f;
		tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight * 0.5f - 0.5f;

		bloomeffect->SetTechnique("brightpass");

		device->SetTexture(0, scenetarget);
		device->SetRenderTarget(0, dssurfaces[0]);

		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		bloomeffect->EndPass();
		bloomeffect->End();

		// STEP 5: downsample before blurring
		bloomeffect->SetTechnique("downsample");
		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);

		for( int i = 1; i < 5; ++i )
		{
			texelsize.x = (float)(2 << i) / (float)screenwidth;
			texelsize.y = (float)(2 << i) / (float)screenheight;

			tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth / ((float)(2 << i)) - 0.5f;
			tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight / ((float)(2 << i)) - 0.5f;

			bloomeffect->SetVector("texelsize", &texelsize);
			bloomeffect->CommitChanges();

			device->SetRenderTarget(0, dssurfaces[i]);
			device->SetTexture(0, dstargets[i - 1]);
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}

		bloomeffect->EndPass();
		bloomeffect->End();

		// STEP 6: blur
		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);

		bloomeffect->SetTechnique("blurx");
		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);

		for( int i = 0; i < 5; ++i )
		{
			texelsize.x = (float)(2 << i) / (float)screenwidth;
			texelsize.y = (float)(2 << i) / (float)screenheight;
			
			tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth / ((float)(2 << i)) - 0.5f;
			tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight / ((float)(2 << i)) - 0.5f;

			bloomeffect->SetVector("texelsize", &texelsize);
			bloomeffect->CommitChanges();

			device->SetRenderTarget(0, blursurfaces[i]);
			device->SetTexture(0, dstargets[i]);
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}

		bloomeffect->EndPass();
		bloomeffect->End();

		// blur on y
		bloomeffect->SetTechnique("blury");
		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);

		for( int i = 0; i < 5; ++i )
		{
			texelsize.x = (float)(2 << i) / (float)screenwidth;
			texelsize.y = (float)(2 << i) / (float)screenheight;
			
			tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth / ((float)(2 << i)) - 0.5f;
			tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight / ((float)(2 << i)) - 0.5f;

			bloomeffect->SetVector("texelsize", &texelsize);
			bloomeffect->CommitChanges();

			device->SetRenderTarget(0, dssurfaces[i]);
			device->SetTexture(0, blurtargets[i]);
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}

		bloomeffect->EndPass();
		bloomeffect->End();

		// STEP 7: ghost
		texelsize.x = 2.0f / (float)screenwidth;
		texelsize.y = 2.0f / (float)screenheight;

		bloomeffect->SetTechnique("ghost");
		bloomeffect->SetVector("texelsize", &texelsize);

		device->SetTexture(0, dstargets[0]);
		device->SetTexture(1, dstargets[1]);
		device->SetTexture(2, dstargets[2]);
		device->SetRenderTarget(0, ghostsurfaces[0]);

		tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth / 2.0f - 0.5f;
		tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight / 2.0f - 0.5f;

		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		bloomeffect->EndPass();
		bloomeffect->End();

		// STEP 8: star
		texelsize.x = 4.0f / (float)screenwidth;
		texelsize.y = 4.0f / (float)screenheight;

		bloomeffect->SetTechnique("star");
		bloomeffect->SetVector("texelsize", &texelsize);

		tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth / 4.0f - 0.5f;
		tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight / 4.0f - 0.5f;

		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);
		
		for( int i = 0; i < 4; ++i )
		{
			bloomeffect->SetInt("stardir", i);

			for( int j = 0; j < 3; ++j )
			{
				bloomeffect->SetInt("starpass", j);
				bloomeffect->CommitChanges();

				int ind = (j % 2);

				device->SetRenderTarget(0, starsurfaces[i][ind]);
				device->SetTexture(0, (j == 0 ? dstargets[1] : startargets[i][1 - ind]));
				device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
			}
		}

		bloomeffect->EndPass();
		bloomeffect->End();

		// combine star textures
		bloomeffect->SetTechnique("starcombine");

		device->SetTexture(0, startargets[0][0]);
		device->SetTexture(1, startargets[1][0]);
		device->SetTexture(2, startargets[2][0]);
		device->SetTexture(3, startargets[3][0]);
		device->SetRenderTarget(0, blursurfaces[1]);

		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		bloomeffect->EndPass();
		bloomeffect->End();

		// STEP 9: combine blur textures
		bloomeffect->SetTechnique("blurcombine");
		
		tmpvert[6] = tmpvert[24] = tmpvert[30] = (float)screenwidth * 0.5f - 0.5f;
		tmpvert[13] = tmpvert[19] = tmpvert[31] = (float)screenheight * 0.5f - 0.5f;

		device->SetTexture(0, dstargets[0]);
		device->SetTexture(1, dstargets[1]);
		device->SetTexture(2, dstargets[2]);
		device->SetTexture(3, dstargets[3]);
		device->SetTexture(4, dstargets[4]);
		device->SetRenderTarget(0, blursurfaces[0]);

		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tmpvert, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		bloomeffect->EndPass();
		bloomeffect->End();

		// STEP 10: final scene
		bloomeffect->SetTechnique("final");

		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		
		device->SetRenderTarget(0, oldtarget);
		device->SetTexture(0, scenetarget);     // scene
		device->SetTexture(1, blurtargets[0]);  // blur
		device->SetTexture(2, blurtargets[1]);  // star
		device->SetTexture(3, ghosttargets[0]); // ghost

		// or color0 = pow(color0, 1.0f / 2.2f); in shader
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, true);

		bloomeffect->Begin(NULL, 0);
		bloomeffect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, vertices, sizeof(D3DXVECTOR4) + sizeof(D3DXVECTOR2));
		}
		bloomeffect->EndPass();
		bloomeffect->End();

		// clean up
		device->SetTexture(1, NULL);
		device->SetTexture(2, NULL);
		device->SetTexture(3, NULL);
		device->SetTexture(4, NULL);
		device->SetTexture(5, NULL);

		device->EndScene();
	}

	oldtarget->Release();
	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
