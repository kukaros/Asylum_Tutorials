//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>

// helper macros
#define TITLE				"Shader tutorial 1: Fixed function pipeline"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern long screenwidth;
extern long screenheight;
extern LPDIRECT3DDEVICE9 device;
extern HWND hwnd;

// tutorial variables
LPD3DXMESH mesh = NULL;
D3DXMATRIX world, view, proj;

HRESULT InitScene()
{
	SetWindowText(hwnd, TITLE);

	if( FAILED(D3DXCreateSphere(device, 1, 30, 30, &mesh, NULL)) )
	{
		MYERROR("Could not create sphere");
		return E_FAIL;
	}

	D3DXVECTOR3 eye(0, 0, -3);
	D3DXVECTOR3 look(0, 0, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)screenwidth / (float)screenheight, 1, 10);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixIdentity(&world);

	D3DMATERIAL9 mat;
	D3DLIGHT9 light;

	mat.Ambient = D3DXCOLOR(0xffffffff);
	mat.Diffuse = D3DXCOLOR(0xff88ff55);
	mat.Specular = D3DXCOLOR(0xffffffff);
	mat.Emissive = D3DXCOLOR((DWORD)0);
	mat.Power = 80.0f;

	memset(&light, 0, sizeof(D3DLIGHT9));

	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Ambient = D3DXCOLOR(0xff222222);
	light.Diffuse = D3DXCOLOR(0xffffffff);
	light.Specular = D3DXCOLOR(0xffffffff);
	light.Direction = D3DXVECTOR3(-1, -1, 0.4f);
	light.Range = 1000.0f;
	light.Attenuation0 = 1;

	device->SetRenderState(D3DRS_LIGHTING, TRUE);
	device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
	device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	device->SetMaterial(&mat);
	device->SetLight(0, &light);
	device->LightEnable(0, TRUE);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_RELEASE(mesh);
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
}
//*************************************************************************************************************
void Update(float delta)
{
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);

	// ez a fixed function pipeline-ra vonatkozik
	device->SetTransform(D3DTS_WORLD, &world);
	device->SetTransform(D3DTS_VIEW, &view);
	device->SetTransform(D3DTS_PROJECTION, &proj);

	if( SUCCEEDED(device->BeginScene()) )
	{
		mesh->DrawSubset(0);
		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
