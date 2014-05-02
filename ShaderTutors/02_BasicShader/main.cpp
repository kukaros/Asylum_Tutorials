//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>

// helper macros
#define TITLE				"Shader tutorial 2: Basic shaders"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern long screenwidth;
extern long screenheight;
extern LPDIRECT3DDEVICE9 device;
extern HWND hwnd;

// tutorial variables
LPD3DXMESH		mesh		= NULL;
LPD3DXEFFECT	effect		= NULL;
D3DXMATRIX		world, view, proj;

HRESULT InitScene()
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;

	SetWindowText(hwnd, TITLE);

	if( FAILED(hr = D3DXCreateSphere(device, 0.5f, 30, 30, &mesh, NULL)) )
	{
		MYERROR("Could not create sphere");
		return hr;
	}

	hr = D3DXCreateEffectFromFileA(device, "../media/shaders/basic.fx", NULL, NULL, D3DXSHADER_DEBUG, NULL, &effect, &errors);

	if( FAILED(hr) )
	{
		if( errors )
		{
			char* str = (char*)errors->GetBufferPointer();
			std::cout << str << "\n\n";

			errors->Release();
			return hr;
		}
	}

	D3DXVECTOR3 eye(0, 0, -1.5f);
	D3DXVECTOR3 look(0, 0, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)screenwidth / (float)screenheight, 1, 10);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixIdentity(&world);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_RELEASE(mesh);
	SAFE_RELEASE(effect);
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
	D3DXMATRIX wvp;

	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);

	D3DXMatrixMultiply(&wvp, &world, &view);
	D3DXMatrixMultiply(&wvp, &wvp, &proj);

	effect->SetMatrix("matWVP", &wvp);

	if( SUCCEEDED(device->BeginScene()) )
	{
		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			mesh->DrawSubset(0);
		}
		effect->EndPass();
		effect->End();

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
