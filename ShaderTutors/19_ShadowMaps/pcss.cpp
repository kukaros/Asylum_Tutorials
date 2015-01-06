
#include <d3dx9.h>

extern LPDIRECT3DDEVICE9				device;
extern LPDIRECT3DTEXTURE9				shadowmap;
extern LPDIRECT3DVERTEXDECLARATION9		vertexdecl;
extern LPD3DXEFFECT						pcss;
extern float							vertices[36];

extern void RenderScene(LPD3DXEFFECT, int);

void RenderWithPCSS(
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

	pcss->SetTechnique("shadowmap");
	pcss->SetMatrix("lightView", &lightview);
	pcss->SetMatrix("lightProj", &lightproj);
	pcss->SetVector("clipPlanes", &clipplanes);

	pcss->Begin(NULL, 0);
	pcss->BeginPass(0);
	{
		RenderScene(pcss, 1); // caster
		RenderScene(pcss, 3); // caster & receiver
	}
	pcss->EndPass();
	pcss->End();

	// STEP 2: render scene
	D3DXVECTOR4 lightradiusls = D3DXVECTOR4(0.5f / clipplanes.z, 0.5f / clipplanes.w, 0, 0);

	//device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	device->SetRenderTarget(0, oldsurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);

	oldsurface->Release();

	device->SetTexture(1, shadowmap);

	pcss->SetTechnique("pcss");
	pcss->SetMatrix("matViewProj", &viewproj);
	pcss->SetMatrix("lightView", &lightview);
	pcss->SetMatrix("lightProj", &lightproj);
	pcss->SetVector("clipPlanes", &clipplanes);
	pcss->SetVector("lightPos", &lightpos);
	pcss->SetVector("eyePos", (D3DXVECTOR4*)&eye);
	pcss->SetVector("texelSize", &texelsize);
	pcss->SetVector("lightRadius", &lightradiusls);

	pcss->Begin(NULL, 0);
	pcss->BeginPass(0);
	{
		RenderScene(pcss, 2); // receiver
		RenderScene(pcss, 3); // caster & receiver
	}
	pcss->EndPass();
	pcss->End();

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
}
