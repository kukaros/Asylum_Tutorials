
#include <d3dx9.h>

extern LPDIRECT3DDEVICE9				device;
extern LPDIRECT3DTEXTURE9				shadowmap;
extern LPDIRECT3DVERTEXDECLARATION9		vertexdecl;
extern LPD3DXEFFECT						expvariance;
extern float							vertices[36];

extern void RenderScene(LPD3DXEFFECT, int);
extern void BlurTexture(LPDIRECT3DTEXTURE9);

void RenderWithExponentialVariance(
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

	expvariance->SetTechnique("shadowmap");
	expvariance->SetMatrix("lightView", &lightview);
	expvariance->SetMatrix("lightProj", &lightproj);
	expvariance->SetVector("clipPlanes", &clipplanes);

	expvariance->Begin(NULL, 0);
	expvariance->BeginPass(0);
	{
		RenderScene(expvariance, 1);
		RenderScene(expvariance, 3);
	}
	expvariance->EndPass();
	expvariance->End();

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

	expvariance->SetTechnique("expvariance");
	expvariance->SetMatrix("matViewProj", &viewproj);
	expvariance->SetMatrix("lightView", &lightview);
	expvariance->SetMatrix("lightProj", &lightproj);
	expvariance->SetVector("clipPlanes", &clipplanes);
	expvariance->SetVector("lightPos", &lightpos);
	expvariance->SetVector("eyePos", (D3DXVECTOR4*)&eye);

	expvariance->Begin(NULL, 0);
	expvariance->BeginPass(0);
	{
		RenderScene(expvariance, 2);
		RenderScene(expvariance, 3);
	}
	expvariance->EndPass();
	expvariance->End();

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
}
