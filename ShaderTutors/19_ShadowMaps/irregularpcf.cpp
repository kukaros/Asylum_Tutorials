
#include <d3dx9.h>

extern LPDIRECT3DDEVICE9				device;
extern LPDIRECT3DTEXTURE9				shadowmap;
extern LPDIRECT3DTEXTURE9				noise;
extern LPDIRECT3DVERTEXDECLARATION9		vertexdecl;
extern LPD3DXEFFECT						pcfirreg;
extern float							vertices[36];

extern void RenderScene(LPD3DXEFFECT, int);

void RenderWithIrregularPCF(
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
	D3DXVECTOR4 noisesize(16.0f, 16.0f, 0, 1);

	// STEP 1: render shadow map
	shadowmap->GetSurfaceLevel(0, &shadowsurface);

	device->GetRenderTarget(0, &oldsurface);
	device->SetRenderTarget(0, shadowsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
	//device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

	shadowsurface->Release();

	pcfirreg->SetTechnique("shadowmap");
	pcfirreg->SetMatrix("lightView", &lightview);
	pcfirreg->SetMatrix("lightProj", &lightproj);
	pcfirreg->SetVector("clipPlanes", &clipplanes);

	pcfirreg->Begin(NULL, 0);
	pcfirreg->BeginPass(0);
	{
		RenderScene(pcfirreg, 1); // caster
		RenderScene(pcfirreg, 3); // caster & receiver
	}
	pcfirreg->EndPass();
	pcfirreg->End();

	// STEP 2: render scene
	//device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	device->SetRenderTarget(0, oldsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);

	oldsurface->Release();

	device->SetTexture(1, shadowmap);
	device->SetTexture(2, noise);

	pcfirreg->SetTechnique("irregular_light");
	//pcfirreg->SetTechnique("irregular_screen");
	pcfirreg->SetMatrix("matViewProj", &viewproj);
	pcfirreg->SetMatrix("lightView", &lightview);
	pcfirreg->SetMatrix("lightProj", &lightproj);
	pcfirreg->SetVector("clipPlanes", &clipplanes);
	pcfirreg->SetVector("lightPos", &lightpos);
	pcfirreg->SetVector("eyePos", (D3DXVECTOR4*)&eye);
	pcfirreg->SetVector("texelSize", &texelsize);
	pcfirreg->SetVector("noiseSize", &noisesize);

	pcfirreg->Begin(NULL, 0);
	pcfirreg->BeginPass(0);
	{
		RenderScene(pcfirreg, 2); // receiver
		RenderScene(pcfirreg, 3); // caster & receiver
	}
	pcfirreg->EndPass();
	pcfirreg->End();

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
}
