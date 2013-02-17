//=============================================================================================================
#include "animatedmesh.h"
#include <iostream>

AnimatedMesh::AnimatedMesh()
{
	Method = SM_Software;
	Effect = NULL;

	device = NULL;
	root = NULL;
	controller = NULL;
	bonetransforms = NULL;

	numanimations = 0;
	maxnumbones = 0;
	currenttime = 0;
}
//=============================================================================================================
AnimatedMesh::~AnimatedMesh()
{
	if( root )
	{
		D3DXFrameDestroy(root, this);
		root = 0;
	}

	if( controller )
	{
		controller->Release();
		controller = 0;
	}

	if( bonetransforms )
	{
		delete[] bonetransforms;
		bonetransforms = NULL;
	}
}
//=============================================================================================================
HRESULT AnimatedMesh::CreateFrame(LPCSTR Name, LPD3DXFRAME* retNewFrame)
{
	*retNewFrame = 0;

	D3DXFRAME_EXTENDED* newFrame = new D3DXFRAME_EXTENDED;
	memset(newFrame, 0, sizeof(D3DXFRAME_EXTENDED));
	
	D3DXMatrixIdentity(&newFrame->TransformationMatrix);
	D3DXMatrixIdentity(&newFrame->exCombinedTransformationMatrix);

	if( Name )
	{
		size_t len = strlen(Name);

		if( len > 0 )
		{
			newFrame->Name = new char[len + 1];
			newFrame->Name[len] = 0;

			memcpy(newFrame->Name, Name, len);
			std::cout << "Added frame: " << newFrame->Name << "\n";
		}
	}
	else
		newFrame->Name = 0;

	newFrame->pMeshContainer = 0;
	newFrame->pFrameSibling = 0;
	newFrame->pFrameFirstChild = 0;
	newFrame->exEnabled = true;

	*retNewFrame = newFrame;
	return S_OK;
}
//=============================================================================================================
HRESULT AnimatedMesh::CreateMeshContainer(
	LPCSTR Name,
	const D3DXMESHDATA* meshData,
	const D3DXMATERIAL* materials,
	const D3DXEFFECTINSTANCE* effectInstances,
	DWORD numMaterials,
	const DWORD* adjacency,
	LPD3DXSKININFO pSkinInfo,
	LPD3DXMESHCONTAINER* retNewMeshContainer)
{
	*retNewMeshContainer = 0;

	D3DXMESHCONTAINER_EXTENDED* newMeshContainer = new D3DXMESHCONTAINER_EXTENDED;
	memset(newMeshContainer, 0, sizeof(D3DXMESHCONTAINER_EXTENDED));

	if( meshData->Type != D3DXMESHTYPE_MESH )
	{
		DestroyMeshContainer(newMeshContainer);

		std::cout << "Only simple meshes are supported\n";
		return E_FAIL;
	}

	newMeshContainer->MeshData.Type = D3DXMESHTYPE_MESH;
	DWORD dwFaces = meshData->pMesh->GetNumFaces();

	newMeshContainer->pAdjacency = new DWORD[dwFaces * 3];
	memcpy(newMeshContainer->pAdjacency, adjacency, sizeof(DWORD) * dwFaces * 3);
	
	LPDIRECT3DDEVICE9 pd3dDevice = 0;
	meshData->pMesh->GetDevice(&pd3dDevice);

	if( Name )
	{
		size_t len = strlen(Name);

		if( len > 0 )
		{
			newMeshContainer->Name = new char[len + 1];
			newMeshContainer->Name[len] = 0;

			memcpy(newMeshContainer->Name, Name, len);
			std::cout << "Added mesh container: " << newMeshContainer->Name << "\n";
		}
		else
			newMeshContainer->Name = 0;
	}
	else
		newMeshContainer->Name = 0;

	newMeshContainer->MeshData.pMesh = meshData->pMesh;
	newMeshContainer->MeshData.pMesh->AddRef();

	newMeshContainer->NumMaterials = max(numMaterials, 1);
	newMeshContainer->exMaterials = new D3DMATERIAL9[newMeshContainer->NumMaterials];
	newMeshContainer->exTextures = new LPDIRECT3DTEXTURE9[newMeshContainer->NumMaterials];

	memset(newMeshContainer->exTextures, 0, sizeof(LPDIRECT3DTEXTURE9) * newMeshContainer->NumMaterials);

	if( numMaterials > 0 )
	{
		for( DWORD i = 0; i < numMaterials; ++i )
		{
			newMeshContainer->exTextures[i] = 0;	
			newMeshContainer->exMaterials[i] = materials[i].MatD3D;

			if( materials[i].pTextureFilename )
			{
				std::string texturePath(materials[i].pTextureFilename);
				
				if( FAILED(D3DXCreateTextureFromFile(pd3dDevice, (Path + texturePath).c_str(), &newMeshContainer->exTextures[i])))
				{
					std::cout << "Could not load texture '" << texturePath << "'\n";
				}
			}
		}
	}
	else
	{
		// create default material
		memset(&newMeshContainer->exMaterials[0], 0, sizeof(D3DMATERIAL9));

		newMeshContainer->exMaterials[0].Diffuse.r = 0.5f;
		newMeshContainer->exMaterials[0].Diffuse.g = 0.5f;
		newMeshContainer->exMaterials[0].Diffuse.b = 0.5f;
		newMeshContainer->exMaterials[0].Specular = newMeshContainer->exMaterials[0].Diffuse;
		newMeshContainer->exTextures[0] = 0;
	}

	if( pSkinInfo )
	{
		newMeshContainer->pSkinInfo = pSkinInfo;
		pSkinInfo->AddRef();

		UINT numBones = pSkinInfo->GetNumBones();

		newMeshContainer->exBoneOffsets = new D3DXMATRIX[numBones];
		newMeshContainer->exFrameCombinedMatrixPointer = new D3DXMATRIX*[numBones];

		for( UINT i = 0; i < numBones; ++i )
			newMeshContainer->exBoneOffsets[i] = *(newMeshContainer->pSkinInfo->GetBoneOffsetMatrix(i));

		GenerateSkinnedMesh(newMeshContainer);
	}
	else
	{
		newMeshContainer->pSkinInfo = 0;
		newMeshContainer->exBoneOffsets = 0;
		newMeshContainer->exSkinMesh = 0;
		newMeshContainer->exFrameCombinedMatrixPointer = 0;
	}

	pd3dDevice->Release();

	if( effectInstances )
	{
		if( effectInstances->pEffectFilename )
			std::cout << "Effect instances not supported\n";
	}
	
	*retNewMeshContainer = newMeshContainer; 
	return S_OK;
}
//=============================================================================================================
HRESULT AnimatedMesh::DestroyFrame(LPD3DXFRAME frameToFree) 
{
	D3DXFRAME_EXTENDED* frame = (D3DXFRAME_EXTENDED*)frameToFree;

	if( frame->Name )
	{
		delete[] frame->Name;
		frame->Name = 0;
	}

	delete frame;
	return S_OK; 
}
//=============================================================================================================
HRESULT AnimatedMesh::DestroyMeshContainer(LPD3DXMESHCONTAINER meshContainerBase)
{
	D3DXMESHCONTAINER_EXTENDED* meshContainer = (D3DXMESHCONTAINER_EXTENDED*)meshContainerBase;

	if( !meshContainer )
		return S_OK;

	if( meshContainer->Name )
	{
		delete[] meshContainer->Name;
		meshContainer->Name = 0;
	}

	if( meshContainer->exMaterials )
	{
		delete[] meshContainer->exMaterials;
		meshContainer->exMaterials = 0;
	}

	if( meshContainer->exTextures )
	{
		for( UINT i = 0; i < meshContainer->NumMaterials; ++i )
		{
			if( meshContainer->exTextures[i] )
			{
				meshContainer->exTextures[i]->Release();
				meshContainer->exTextures[i] = 0;
			}
		}
	}

	if( meshContainer->exTextures )
	{
		delete[] meshContainer->exTextures;
		meshContainer->exTextures = 0;
	}

	if( meshContainer->pAdjacency )
	{
		delete[] meshContainer->pAdjacency;
		meshContainer->pAdjacency = 0;
	}
	
	if( meshContainer->exBoneOffsets )
	{
		delete[] meshContainer->exBoneOffsets;
		meshContainer->exBoneOffsets = 0;
	}
	
	if( meshContainer->exFrameCombinedMatrixPointer )
	{
		delete[] meshContainer->exFrameCombinedMatrixPointer;
		meshContainer->exFrameCombinedMatrixPointer = 0;
	}
	
	if( meshContainer->exSkinMesh )
	{
		meshContainer->exSkinMesh->Release();
		meshContainer->exSkinMesh = 0;
	}
	
	if( meshContainer->MeshData.pMesh )
	{
		meshContainer->MeshData.pMesh->Release();
		meshContainer->MeshData.pMesh = 0;
	}
		
	if( meshContainer->pSkinInfo )
	{
		meshContainer->pSkinInfo->Release();
		meshContainer->pSkinInfo = 0;
	}
	
	delete meshContainer;
	meshContainer = 0;

	return S_OK;
}
//=============================================================================================================
void AnimatedMesh::GenerateSkinnedMesh(D3DXMESHCONTAINER_EXTENDED* meshContainer)
{
	if( Method == SM_Shader )
	{
		if( !meshContainer->MeshData.pMesh )
			return;
		
		DWORD Flags = D3DXMESHOPT_VERTEXCACHE|D3DXMESH_MANAGED;
		HRESULT hr;

		meshContainer->exNumBoneMatrices = min(MAX_MATRICES, meshContainer->pSkinInfo->GetNumBones());

		if( meshContainer->exSkinMesh )
		{
			meshContainer->exSkinMesh->Release();
			meshContainer->exSkinMesh = 0;
		}

		if( meshContainer->exBoneCombinationBuff )
		{
			meshContainer->exBoneCombinationBuff->Release();
			meshContainer->exBoneCombinationBuff = 0;
		}
		
		hr = meshContainer->pSkinInfo->ConvertToIndexedBlendedMesh(
			meshContainer->MeshData.pMesh,
			Flags,
			meshContainer->exNumBoneMatrices,
			meshContainer->pAdjacency,
			NULL, NULL, NULL,
			&meshContainer->exNumInfl,
			&meshContainer->exNumAttributeGroups,
			&meshContainer->exBoneCombinationBuff,
			&meshContainer->exSkinMesh);
		
		if( FAILED(hr) )
		{
			std::cout << "Could not convert to blended mesh\n";
			return;
		}

		// a béta mezöket a skinninghez használja a ffp
		DWORD NewFVF = (meshContainer->exSkinMesh->GetFVF() & D3DFVF_POSITION_MASK)|D3DFVF_NORMAL|D3DFVF_TEX1|D3DFVF_LASTBETA_UBYTE4;

		if( NewFVF != meshContainer->exSkinMesh->GetFVF() )
		{
			LPD3DXMESH pMesh;

			hr = meshContainer->exSkinMesh->CloneMeshFVF(
				meshContainer->exSkinMesh->GetOptions(), NewFVF, device, &pMesh);

			if( FAILED(hr) )
			{
				std::cout << "Could not clone fvf\n";
			}
			else
			{
				meshContainer->exSkinMesh->Release();
				meshContainer->exSkinMesh = pMesh;
				pMesh = NULL;
			}
		}

		D3DVERTEXELEMENT9 pDecl[MAX_FVF_DECL_SIZE];
		LPD3DVERTEXELEMENT9 pDeclCur;

		hr = meshContainer->exSkinMesh->GetDeclaration(pDecl);

		if( FAILED(hr) )
		{
			std::cout << "Could not get declaration\n";
			return;
		}

		// egyes kártyák nem tudnak ubyte4-et
		pDeclCur = pDecl;

		while( pDeclCur->Stream != 0xff )
		{
			if( (pDeclCur->Usage == D3DDECLUSAGE_BLENDINDICES) && (pDeclCur->UsageIndex == 0) )
				pDeclCur->Type = D3DDECLTYPE_D3DCOLOR;

			pDeclCur++;
		}

		hr = meshContainer->exSkinMesh->UpdateSemantics(pDecl);

		if( FAILED(hr) )
		{
			std::cout << "Could not update semantics\n";
			return;
		}

		maxnumbones = max(maxnumbones, (int)meshContainer->pSkinInfo->GetNumBones());
	}
	else if( Method == SM_Software )
	{
		if( meshContainer->MeshData.pMesh )
		{
			D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];

			if( FAILED(meshContainer->MeshData.pMesh->GetDeclaration(decl)) )
				return;

			meshContainer->MeshData.pMesh->CloneMesh(D3DXMESH_MANAGED, decl, device, &meshContainer->exSkinMesh);
		}

		maxnumbones = max(maxnumbones, (int)meshContainer->pSkinInfo->GetNumBones());
	}
}
//=============================================================================================================
HRESULT AnimatedMesh::Load(LPDIRECT3DDEVICE9 device, const std::string& file)
{
	if( !this->device )
		this->device = device;

	HRESULT hr = D3DXLoadMeshHierarchyFromXA(file.c_str(), D3DXMESH_MANAGED, device, this, NULL, &root, &controller);

	if( SUCCEEDED(hr) )
	{
		if( controller )
		{
			numanimations = controller->GetNumAnimationSets();
			currentanim = 0;
			currenttrack = 0;

			std::cout << "Number of animations: " << numanimations << "\n";
		}
		else
			std::cout << "No animation controller\n";

		if( root )
		{
			SetupMatrices((D3DXFRAME_EXTENDED*)root, NULL);

			if( bonetransforms )
				delete[] bonetransforms;

			bonetransforms = new D3DXMATRIX[maxnumbones];
		}
	}

	return hr;
}
//=============================================================================================================
void AnimatedMesh::SetupMatrices(D3DXFRAME_EXTENDED* frame, LPD3DXMATRIX parent)
{
	D3DXMESHCONTAINER_EXTENDED* pMesh = (D3DXMESHCONTAINER_EXTENDED*)frame->pMeshContainer;

	if( pMesh )
	{
		if( pMesh->pSkinInfo )
		{
			for( DWORD i = 0; i < pMesh->pSkinInfo->GetNumBones(); ++i )
			{
				D3DXFRAME_EXTENDED* pTempFrame =
					(D3DXFRAME_EXTENDED*)D3DXFrameFind(root, pMesh->pSkinInfo->GetBoneName(i));

				pMesh->exFrameCombinedMatrixPointer[i] = &pTempFrame->exCombinedTransformationMatrix;
			}
		}
	}

	if( frame->pFrameSibling )
		SetupMatrices((D3DXFRAME_EXTENDED*)frame->pFrameSibling, parent);

	if( frame->pFrameFirstChild )
		SetupMatrices((D3DXFRAME_EXTENDED*)frame->pFrameFirstChild, &frame->exCombinedTransformationMatrix);
}
//=============================================================================================================
void AnimatedMesh::UpdateMatrices(LPD3DXFRAME frame, LPD3DXMATRIX parent)
{
	D3DXFRAME_EXTENDED* currentFrame = (D3DXFRAME_EXTENDED*)frame;

	if( parent )
	{
		D3DXMatrixMultiply(
			&currentFrame->exCombinedTransformationMatrix,
			&currentFrame->TransformationMatrix, parent);
	}
	else
	{
		currentFrame->exCombinedTransformationMatrix =
			currentFrame->TransformationMatrix;
	}

	if( currentFrame->pFrameSibling )
		UpdateMatrices(currentFrame->pFrameSibling, parent);

	if( currentFrame->pFrameFirstChild )
		UpdateMatrices(currentFrame->pFrameFirstChild, &currentFrame->exCombinedTransformationMatrix);
}
//=============================================================================================================
void AnimatedMesh::DrawFrame(LPD3DXFRAME frame)
{
	if( ((D3DXFRAME_EXTENDED*)frame)->exEnabled )
	{
		LPD3DXMESHCONTAINER meshContainer = frame->pMeshContainer;

		while( meshContainer )
		{
			DrawMeshContainer(meshContainer, frame);
			meshContainer = meshContainer->pNextMeshContainer;
		}

		if( frame->pFrameFirstChild )
			DrawFrame(frame->pFrameFirstChild);
	}

	if( frame->pFrameSibling )
		DrawFrame(frame->pFrameSibling);
}
//=============================================================================================================
void AnimatedMesh::DrawMeshContainer(LPD3DXMESHCONTAINER meshContainerBase, LPD3DXFRAME frameBase)
{
	D3DXFRAME_EXTENDED* frame = (D3DXFRAME_EXTENDED*)frameBase;		
	D3DXMESHCONTAINER_EXTENDED* meshContainer = (D3DXMESHCONTAINER_EXTENDED*)meshContainerBase;

	if( Method == SM_Software )
	{
		if( meshContainer->pSkinInfo )
		{
			// skinned
			DWORD Bones = meshContainer->pSkinInfo->GetNumBones();
			
			for( DWORD i = 0; i < Bones; ++i )
				D3DXMatrixMultiply(&bonetransforms[i], &meshContainer->exBoneOffsets[i], meshContainer->exFrameCombinedMatrixPointer[i]);
			
			void* srcPtr = 0;
			void* destPtr = 0;

			meshContainer->MeshData.pMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&srcPtr);
			meshContainer->exSkinMesh->LockVertexBuffer(0, (void**)&destPtr);
			meshContainer->pSkinInfo->UpdateSkinnedMesh(bonetransforms, NULL, srcPtr, destPtr);
			meshContainer->exSkinMesh->UnlockVertexBuffer();
			meshContainer->MeshData.pMesh->UnlockVertexBuffer();

			D3DXMATRIX id(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
			device->SetTransform(D3DTS_WORLD, &id);

			for( unsigned int iMaterial = 0; iMaterial < meshContainer->NumMaterials; ++iMaterial )
			{
				device->SetMaterial(&meshContainer->exMaterials[iMaterial]);
				device->SetTexture(0, meshContainer->exTextures[iMaterial]);

				LPD3DXMESH pDrawMesh = meshContainer->exSkinMesh;
				pDrawMesh->DrawSubset(iMaterial);
			}
		}
		else
		{
			// normal
			device->SetTransform(D3DTS_WORLD, &frame->exCombinedTransformationMatrix);

			for( unsigned int iMaterial = 0; iMaterial < meshContainer->NumMaterials; ++iMaterial )
			{
				device->SetMaterial(&meshContainer->exMaterials[iMaterial]);
				device->SetTexture(0, meshContainer->exTextures[iMaterial]);

				LPD3DXMESH pDrawMesh = meshContainer->MeshData.pMesh;
				pDrawMesh->DrawSubset(iMaterial);
			}
		}
	}
	else if( Method == SM_Shader )
	{
		if( meshContainer->exBoneCombinationBuff )
		{
			// skinned
			LPD3DXBONECOMBINATION pBoneComb =
				reinterpret_cast<LPD3DXBONECOMBINATION>(meshContainer->exBoneCombinationBuff->GetBufferPointer());

			for( DWORD iAttrib = 0; iAttrib < meshContainer->exNumAttributeGroups; ++iAttrib )
			{
				for( DWORD iPaletteEntry = 0; iPaletteEntry < meshContainer->exNumBoneMatrices; ++iPaletteEntry )
				{
					DWORD iMatrixIndex = pBoneComb[iAttrib].BoneId[iPaletteEntry];

					if( iMatrixIndex != UINT_MAX )
					{
						D3DXMatrixMultiply(&bonetransforms[iPaletteEntry], &meshContainer->exBoneOffsets[iMatrixIndex],
							meshContainer->exFrameCombinedMatrixPointer[iMatrixIndex]);
					}
				}

				Effect->SetMatrixArray("matBones", bonetransforms, meshContainer->exNumBoneMatrices);
				Effect->SetInt("numBones", meshContainer->exNumInfl - 1);

				device->SetTexture(0, meshContainer->exTextures[pBoneComb[iAttrib].AttribId]);

				Effect->Begin(NULL, D3DXFX_DONOTSAVESTATE);
				Effect->BeginPass(0);

				meshContainer->exSkinMesh->DrawSubset(iAttrib);

				Effect->EndPass();
				Effect->End();

				device->SetVertexShader(NULL);
			}
		}
		else
		{
			// normal
			device->SetTransform(D3DTS_WORLD, &frame->exCombinedTransformationMatrix);

			for( unsigned int iMaterial = 0; iMaterial < meshContainer->NumMaterials; ++iMaterial )
			{
				device->SetMaterial(&meshContainer->exMaterials[iMaterial]);
				device->SetTexture(0, meshContainer->exTextures[iMaterial]);

				LPD3DXMESH pDrawMesh = meshContainer->MeshData.pMesh;
				pDrawMesh->DrawSubset(iMaterial);
			}
		}
	}
}
//=============================================================================================================
void AnimatedMesh::SetAnimation(UINT index)
{
	if( index >= numanimations || index == currentanim )
		return;

	LPD3DXANIMATIONSET set = NULL;
	UINT newtrack = (currenttrack == 0 ? 1 : 0);
	double transitiontime = 0.25f;

	controller->GetAnimationSet(index, &set);
	controller->SetTrackAnimationSet(newtrack, set);

	set->Release();	

	controller->UnkeyAllTrackEvents(currenttrack);
	controller->UnkeyAllTrackEvents(newtrack);

	// sima átmenet két animáció között
	controller->KeyTrackEnable(currenttrack, false, currenttime + transitiontime);
	controller->KeyTrackSpeed(currenttrack, 0.0f, currenttime, transitiontime, D3DXTRANSITION_LINEAR);
	controller->KeyTrackWeight(currenttrack, 0.0f, currenttime, transitiontime, D3DXTRANSITION_LINEAR);

	controller->SetTrackEnable(newtrack, true);
	controller->KeyTrackSpeed(newtrack, 1.0f, currenttime, transitiontime, D3DXTRANSITION_LINEAR);
	controller->KeyTrackWeight(newtrack, 1.0f, currenttime, transitiontime, D3DXTRANSITION_LINEAR);

	currenttrack = newtrack;
	currentanim = index;
}
//=============================================================================================================
void AnimatedMesh::Update(float delta, LPD3DXMATRIX world)
{
	if( controller )
		HRESULT hr = controller->AdvanceTime(delta, NULL);

	if( root )
		UpdateMatrices(root, world);

	currenttime += delta;
}
//=============================================================================================================
void AnimatedMesh::Draw()
{
	if( root )
		DrawFrame(root);
}
//=============================================================================================================
void AnimatedMesh::EnableFrame(const std::string& name, bool enable)
{
	D3DXFRAME_EXTENDED* frame = (D3DXFRAME_EXTENDED*)D3DXFrameFind(root, name.c_str());

	if( frame )
		frame->exEnabled = enable;
}
//=============================================================================================================
void AnimatedMesh::NextAnimation()
{
	SetAnimation((currentanim + 1) % numanimations);
}
//=============================================================================================================
