
#ifndef _DXEXT_H_
#define _DXEXT_H_

#include <d3dx9.h>
#include <string>

class DXObject
{
private:
	IDirect3DDevice9*		device;
	ID3DXMesh*				mesh;
	D3DXMATERIAL*			materials;
	IDirect3DTexture9**		textures;
	DWORD					nummaterials;

	void Clean();

public:
	DXObject(IDirect3DDevice9* d3ddevice);
	~DXObject();

	bool Load(const std::string& file);
	bool Save(const std::string& file);

	void Draw(LPD3DXEFFECT effect = NULL);
	void DrawSubset(DWORD subset);
};

class DXPointLight
{
public:
	LPDIRECT3DCUBETEXTURE9	ShadowMap;
	LPDIRECT3DCUBETEXTURE9	Blur;

	D3DXVECTOR4				Color;
	D3DXVECTOR3				Position;
	float					Radius;

	void GetScissorRect(RECT& out, const D3DXMATRIX& view, const D3DXMATRIX& proj, int w, int h) const;
};

class DXDirectionalLight
{
public:
	LPDIRECT3DTEXTURE9		ShadowMap;
	LPDIRECT3DTEXTURE9		Blur;

	D3DXMATRIX				ViewProj;
	D3DXVECTOR4				Color;
	D3DXVECTOR4				Direction;
};

HRESULT DXLoadMeshFromQM(LPCTSTR file, DWORD options, LPDIRECT3DDEVICE9 d3ddevice, D3DXMATERIAL** materials, DWORD* nummaterials, LPD3DXMESH* mesh);
HRESULT DXSaveMeshToQM(LPCTSTR file, LPD3DXMESH mesh, D3DXMATERIAL* materials, DWORD nummaterials);
HRESULT DXCreateEffect(LPCTSTR file, LPDIRECT3DDEVICE9 d3ddevice, LPD3DXEFFECT* out);
HRESULT DXGenTangentFrame(LPDIRECT3DDEVICE9 d3ddevice, LPD3DXMESH* mesh);

void DXRenderText(const std::string& str, LPDIRECT3DTEXTURE9 tex, DWORD width, DWORD height);

#endif
