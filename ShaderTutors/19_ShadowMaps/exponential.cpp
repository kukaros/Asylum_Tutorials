
#include <d3dx9.h>

extern LPDIRECT3DDEVICE9				device;
extern LPDIRECT3DTEXTURE9				shadowmap;
extern LPDIRECT3DVERTEXDECLARATION9		vertexdecl;
extern LPD3DXEFFECT						exponential;
extern float							vertices[36];

extern void RenderScene(LPD3DXEFFECT, int);
extern void BlurTexture(LPDIRECT3DTEXTURE9);

void RenderWithExponential(
	const D3DXMATRIX& viewproj,
	const D3DXVECTOR3& eye,
	const D3DXMATRIX& lightview,
	const D3DXMATRIX& lightproj,
	const D3DXVECTOR4& lightpos,
	const D3DXVECTOR4& clipplanes)
{
	LPDIRECT3DSURFACE9 oldsurface		= NULL;
	LPDIRECT3DSURFACE9 shadowsurface	= NULL;

	// STEP 1: render shadow map
	shadowmap->GetSurfaceLevel(0, &shadowsurface);

	device->GetRenderTarget(0, &oldsurface);
	device->SetRenderTarget(0, shadowsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);

	shadowsurface->Release();

	exponential->SetTechnique("shadowmap");
	exponential->SetMatrix("lightView", &lightview);
	exponential->SetMatrix("lightProj", &lightproj);
	exponential->SetVector("clipPlanes", &clipplanes);

	exponential->Begin(NULL, 0);
	exponential->BeginPass(0);
	{
		RenderScene(exponential, 1);
		RenderScene(exponential, 3);
	}
	exponential->EndPass();
	exponential->End();

	// STEP 2: apply filter
	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	BlurTexture(shadowmap);
	device->SetRenderState(D3DRS_ZENABLE, TRUE);

	// STEP 3: render scene
	device->SetRenderTarget(0, oldsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);

	oldsurface->Release();

	device->SetTexture(1, shadowmap);

	exponential->SetTechnique("exponential");
	exponential->SetMatrix("matViewProj", &viewproj);
	exponential->SetMatrix("lightView", &lightview);
	exponential->SetMatrix("lightProj", &lightproj);
	exponential->SetVector("clipPlanes", &clipplanes);
	exponential->SetVector("lightPos", &lightpos);
	exponential->SetVector("eyePos", (D3DXVECTOR4*)&eye);

	exponential->Begin(NULL, 0);
	exponential->BeginPass(0);
	{
		RenderScene(exponential, 2);
		RenderScene(exponential, 3);
	}
	exponential->EndPass();
	exponential->End();

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
}
