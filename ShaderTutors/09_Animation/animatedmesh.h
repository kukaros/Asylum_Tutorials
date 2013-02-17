//=============================================================================================================
#ifndef _ANIMATEDMESH_H_
#define _ANIMATEDMESH_H_

#include <d3dx9.h>
#include <string>

#define MAX_MATRICES 26

struct D3DXMESHCONTAINER_EXTENDED : public D3DXMESHCONTAINER
{
	IDirect3DTexture9**	exTextures;
	D3DMATERIAL9*		exMaterials;
	ID3DXMesh*			exSkinMesh;
	D3DXMATRIX*			exBoneOffsets;
	D3DXMATRIX**		exFrameCombinedMatrixPointer;
	DWORD				exNumBoneMatrices;
	DWORD				exNumAttributeGroups;
	DWORD				exNumInfl;
	LPD3DXBUFFER		exBoneCombinationBuff;
};

struct D3DXFRAME_EXTENDED : public D3DXFRAME
{
	D3DXMATRIX exCombinedTransformationMatrix;
	bool exEnabled;
};

enum SkinningMethod
{
	SM_Software,
	SM_Shader
};

class AnimatedMesh : public ID3DXAllocateHierarchy
{
protected:
	LPDIRECT3DDEVICE9			device;
	LPD3DXANIMATIONCONTROLLER	controller;
	LPD3DXFRAME					root;
	D3DXMATRIX*					bonetransforms;		/*!< \brief Ide számoljuk ki rajzoláskor a végsö trafót */

	DWORD maxnumbones;
	UINT numanimations;
	UINT currentanim;
	UINT currenttrack;
	double currenttime;

	void GenerateSkinnedMesh(D3DXMESHCONTAINER_EXTENDED* meshContainer);
	void SetupMatrices(D3DXFRAME_EXTENDED* frame, LPD3DXMATRIX parent);
	void UpdateMatrices(LPD3DXFRAME frame, LPD3DXMATRIX parent);
	void DrawFrame(LPD3DXFRAME frame);
	void DrawMeshContainer(LPD3DXMESHCONTAINER meshContainerBase, LPD3DXFRAME frameBase);

public:
	STDMETHOD(CreateMeshContainer)(
		LPCSTR Name, const D3DXMESHDATA* meshData, const D3DXMATERIAL* materials, const D3DXEFFECTINSTANCE* effectInstances,
		DWORD numMaterials, const DWORD* adjacency, LPD3DXSKININFO skinInfo, LPD3DXMESHCONTAINER* retNewMeshContainer );

	STDMETHOD(CreateFrame)(LPCSTR Name, LPD3DXFRAME* retNewFrame);
	STDMETHOD(DestroyFrame)(LPD3DXFRAME frameToFree);
	STDMETHOD(DestroyMeshContainer)(LPD3DXMESHCONTAINER meshContainerToFree);

public:
	SkinningMethod Method;
	std::string Path;
	LPD3DXEFFECT Effect;

	AnimatedMesh();
	~AnimatedMesh();

	HRESULT Load(LPDIRECT3DDEVICE9 device, const std::string& file);

	void SetAnimation(UINT index);
	void NextAnimation();
	void Update(float delta, LPD3DXMATRIX world);
	void Draw();
	void EnableFrame(const std::string& name, bool enable);
};

#endif
//=============================================================================================================
