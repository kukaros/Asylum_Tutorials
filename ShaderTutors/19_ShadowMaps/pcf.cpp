
#include <d3dx9.h>

extern LPDIRECT3DDEVICE9				device;
extern LPDIRECT3DTEXTURE9				shadowmap;
extern LPDIRECT3DVERTEXDECLARATION9		vertexdecl;
extern LPD3DXEFFECT						pcf5x5;
extern float							vertices[36];

extern void RenderScene(LPD3DXEFFECT, int);

void RenderWithPCF(
	const D3DXMATRIX& viewproj,
	const D3DXVECTOR3& eye,
	const D3DXMATRIX& lightview,
	const D3DXMATRIX& lightproj,
	const D3DXVECTOR4& lightpos,
	const D3DXVECTOR4& clipplanes,
	const D3DXVECTOR4& texelsize)
{
	LPDIRECT3DSURFACE9 oldsurface		= NULL;
	LPDIRECT3DSURFACE9 shadowsurface	= NULL;

	// STEP 1: render shadow map
	shadowmap->GetSurfaceLevel(0, &shadowsurface);

	device->GetRenderTarget(0, &oldsurface);
	device->SetRenderTarget(0, shadowsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
	//device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

	shadowsurface->Release();

	pcf5x5->SetTechnique("shadowmap");
	pcf5x5->SetMatrix("lightView", &lightview);
	pcf5x5->SetMatrix("lightProj", &lightproj);
	pcf5x5->SetVector("clipPlanes", &clipplanes);

	pcf5x5->Begin(NULL, 0);
	pcf5x5->BeginPass(0);
	{
		RenderScene(pcf5x5, 1);
		RenderScene(pcf5x5, 3);
	}
	pcf5x5->EndPass();
	pcf5x5->End();

	// STEP 2: render scene
	//device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	device->SetRenderTarget(0, oldsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);

	oldsurface->Release();

	device->SetTexture(1, shadowmap);

	pcf5x5->SetTechnique("pcf5x5");
	pcf5x5->SetMatrix("matViewProj", &viewproj);
	pcf5x5->SetMatrix("lightView", &lightview);
	pcf5x5->SetMatrix("lightProj", &lightproj);
	pcf5x5->SetVector("clipPlanes", &clipplanes);
	pcf5x5->SetVector("lightPos", &lightpos);
	pcf5x5->SetVector("eyePos", (D3DXVECTOR4*)&eye);
	pcf5x5->SetVector("texelSize", &texelsize);

	pcf5x5->Begin(NULL, 0);
	pcf5x5->BeginPass(0);
	{
		RenderScene(pcf5x5, 2);
		RenderScene(pcf5x5, 3);
	}
	pcf5x5->EndPass();
	pcf5x5->End();

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
}
