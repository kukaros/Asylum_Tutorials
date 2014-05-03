//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>

#include "../common/animatedmesh.h"

// helper macros
#define TITLE				"Shader tutorial 9: Skeletal animation"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern long screenwidth;
extern long screenheight;
extern LPDIRECT3DDEVICE9 device;
extern HWND hwnd;

// tutorial variables
AnimatedMesh		mesh;
LPD3DXEFFECT		effect		= NULL;
D3DXMATRIX			world, view, proj;

D3DVERTEXELEMENT9 elem[] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
	{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	D3DDECL_END()
};

HRESULT InitScene()
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;

	SetWindowText(hwnd, TITLE);

	hr = D3DXCreateEffectFromFileA(device, "../media/shaders/skinning.fx", NULL, NULL, D3DXSHADER_DEBUG, NULL, &effect, &errors);
	
	if( FAILED(hr) )
	{
		if( errors )
		{
			char* str = (char*)errors->GetBufferPointer();
			std::cout << str << "\n\n";

			errors->Release();
		}

		MYERROR("Could not create effect");
		return hr;
	}

	// load animated model
	mesh.Effect = effect;
	mesh.Method = SM_Shader;
	mesh.Path = "../media/meshes/dwarf/";
	
	if( FAILED(hr = mesh.Load(device, "../media/meshes/dwarf/dwarf.X")) )
	{
		MYERROR("Could not load dwarf");
		return hr;
	}

	// all the possible skins are contained in this file => turn off some
	mesh.SetAnimation(6);
	mesh.EnableFrame("LOD0_attachment_weapon3", false);
	mesh.EnableFrame("LOD0_attachment_weapon2", false);
	mesh.EnableFrame("LOD0_attachment_shield3", false);
	mesh.EnableFrame("LOD0_attachment_shield2", false);
	mesh.EnableFrame("LOD0_attachment_head2", false);
	mesh.EnableFrame("LOD0_attachment_head1", false);
	mesh.EnableFrame("LOD0_attachment_torso2", false);
	mesh.EnableFrame("LOD0_attachment_torso3", false);
	mesh.EnableFrame("LOD0_attachment_legs2", false);
	mesh.EnableFrame("LOD0_attachment_legs3", false);
	mesh.EnableFrame("LOD0_attachment_Lpads1", false);
	mesh.EnableFrame("LOD0_attachment_Lpads2", false);
	mesh.EnableFrame("Rshoulder", false);
	
	D3DXVECTOR3 eye(0.5f, 1.0f, -1.5f);
	D3DXVECTOR3 look(0, 0.7f, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)screenwidth / (float)screenheight, 0.1f, 100);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixIdentity(&world);

	std::cout << "Press Space to cycle animations\n";

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	// don't do this, ever!!!
	mesh.~AnimatedMesh();

	SAFE_RELEASE(effect);
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	if( wparam == VK_SPACE )
		mesh.NextAnimation();
}
//*************************************************************************************************************
void Update(float delta)
{
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	D3DXMATRIX viewproj;

	D3DXMatrixMultiply(&viewproj, &view, &proj);
	D3DXMatrixScaling(&world, 0.1f, 0.1f, 0.1f);

	time += elapsedtime;

	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);
	device->SetTransform(D3DTS_WORLD, &world);
	device->SetTransform(D3DTS_VIEW, &view);
	device->SetTransform(D3DTS_PROJECTION, &proj);

	mesh.Update(elapsedtime, &world);

	effect->SetMatrix("matViewProj", &viewproj);

	if( SUCCEEDED(device->BeginScene()) )
	{
		mesh.Draw();
		device->EndScene();
	}
	
	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
