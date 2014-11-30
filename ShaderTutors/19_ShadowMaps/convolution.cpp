
#include <d3dx9.h>

extern LPDIRECT3DDEVICE9				device;
extern LPDIRECT3DTEXTURE9				shadowmap;
extern LPDIRECT3DTEXTURE9				sincoeffs;
extern LPDIRECT3DTEXTURE9				coscoeffs;
extern LPDIRECT3DVERTEXDECLARATION9		vertexdecl;
extern LPD3DXEFFECT						convolution;
extern float							vertices[36];

extern void RenderScene(LPD3DXEFFECT, int);
extern void BlurTexture(LPDIRECT3DTEXTURE9);

void RenderWithConvolution(
	const D3DXMATRIX& viewproj,
	const D3DXVECTOR3& eye,
	const D3DXMATRIX& lightview,
	const D3DXMATRIX& lightproj,
	const D3DXVECTOR4& lightpos,
	const D3DXVECTOR4& clipplanes)
{
	LPDIRECT3DSURFACE9 oldsurface		= NULL;
	LPDIRECT3DSURFACE9 shadowsurface	= NULL;
	LPDIRECT3DSURFACE9 sinsurface		= NULL;
	LPDIRECT3DSURFACE9 cossurface		= NULL;

	// STEP 1: render shadow map
	shadowmap->GetSurfaceLevel(0, &shadowsurface);

	device->GetRenderTarget(0, &oldsurface);
	device->SetRenderTarget(0, shadowsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);

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
	device->SetTexture(1, shadowmap);

	sinsurface->Release();
	cossurface->Release();

	convolution->SetTechnique("evalbasis");

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

	device->SetTexture(1, shadowmap);
	device->SetTexture(2, sincoeffs);
	device->SetTexture(3, coscoeffs);

	convolution->SetTechnique("convolution");
	convolution->SetMatrix("matViewProj", &viewproj);
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
}
