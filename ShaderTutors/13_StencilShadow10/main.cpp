//*************************************************************************************************************
#include <d3d10.h>
#include <d3dx10.h>
#include <iostream>

#include "../common/common.h"

#define NUM_OBJECTS			3

// helper macros
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern ID3D10Device* device;
extern IDXGISwapChain* swapchain;
extern ID3D10RenderTargetView* rendertargetview;
extern ID3D10DepthStencilView* depthstencilview;

extern long		screenwidth;
extern long		screenheight;
extern short	mousedx;
extern short	mousedy;
extern short	mousedown;

extern HRESULT LoadMeshFromQM(LPCTSTR file, DWORD options, ID3DX10Mesh** mesh);

struct ShadowCaster
{
	D3DXMATRIX		world;
	ID3DX10Mesh*	object;
	ID3DX10Mesh*	caster;

	ShadowCaster()
	{
		object = caster = 0;
		D3DXMatrixIdentity(&world);
	};
};

// tutorial variables
ID3D10Effect*				specular		= 0;
ID3D10Effect*				volume			= 0;
ID3DX10Mesh*				receiver		= 0;
ID3D10InputLayout*			vertexlayout1	= 0;
ID3D10InputLayout*			vertexlayout2	= 0;
ID3D10ShaderResourceView*	texture1		= 0;
ID3D10ShaderResourceView*	texture2		= 0;
ID3D10EffectTechnique*		technique1		= 0;
ID3D10EffectTechnique*		technique2		= 0;
ID3D10BlendState*			noblend			= 0;
ID3D10BlendState*			alphablend		= 0;

state<D3DXVECTOR2>			cameraangle;
state<D3DXVECTOR2>			lightangle;
ShadowCaster				objects[NUM_OBJECTS];

HRESULT InitScene()
{
	ID3D10Blob*	errors		= NULL;
	UINT		hlslflags	= D3D10_SHADER_ENABLE_STRICTNESS;
	HRESULT		hr;

#ifdef _DEBUG
	hlslflags |= D3D10_SHADER_DEBUG;
#endif

	MYVALID(D3DX10CreateShaderResourceViewFromFile(device, "../media/textures/marble.dds", 0, 0, &texture1, 0));
	MYVALID(D3DX10CreateShaderResourceViewFromFile(device, "../media/textures/wood2.jpg", 0, 0, &texture2, 0));

	hr = D3DX10CreateEffectFromFile("../media/shaders10/blinnphong10.fx", 0, 0, "fx_4_0", hlslflags, 0, device, 0, 0, &specular, &errors, 0);

	if( errors )
	{
		char* str = (char*)errors->GetBufferPointer();
		std::cout << str << "\n";
	}

	if( FAILED(hr) )
		return hr;

	hr = D3DX10CreateEffectFromFile("../media/shaders10/shadowvolume10.fx", 0, 0, "fx_4_0", hlslflags, 0, device, 0, 0, &volume, &errors, 0);

	if( errors )
	{
		char* str = (char*)errors->GetBufferPointer();
		std::cout << str << "\n";
	}

	if( FAILED(hr) )
		return hr;

	technique1 = specular->GetTechniqueByName("specular");
	technique2 = volume->GetTechniqueByName("extrude");

	D3D10_PASS_DESC					passdesc;
	const D3D10_INPUT_ELEMENT_DESC*	decl = 0;
	UINT							declsize = 0;

	// receiver
	MYVALID(LoadMeshFromQM("../media/meshes10/box.qm", 0, &receiver));

	receiver->GetVertexDescription(&decl, &declsize);
	technique1->GetPassByIndex(0)->GetDesc(&passdesc);

	MYVALID(device->CreateInputLayout(decl, declsize, passdesc.pIAInputSignature, passdesc.IAInputSignatureSize, &vertexlayout1));

	// casters
	MYVALID(LoadMeshFromQM("../media/meshes10/sphere.qm", 0, &objects[1].object));
	MYVALID(LoadMeshFromQM("../media/meshes10/lshape.qm", 0, &objects[2].object));

	MYVALID(LoadMeshFromQM("../media/meshes10/collisionbox.qm", 0, &objects[0].caster));
	MYVALID(LoadMeshFromQM("../media/meshes10/collisionsphere.qm", 0, &objects[1].caster));
	MYVALID(LoadMeshFromQM("../media/meshes10/collisionlshape.qm", 0, &objects[2].caster));

	objects[0].object = receiver;
	receiver->AddRef();

	// generate adjacency
	objects[0].caster->GenerateGSAdjacency();
	objects[1].caster->GenerateGSAdjacency();
	objects[2].caster->GenerateGSAdjacency();

	ID3DX10MeshBuffer*	adjbuffer	= 0;
	WORD*				data		= 0;
	SIZE_T				datasize	= 0;

	objects[0].caster->GetAdjacencyBuffer(&adjbuffer);
	objects[0].caster->Discard(D3DX10_MESH_DISCARD_DEVICE_BUFFERS);

	adjbuffer->Map((void**)&data, &datasize);
	{
		objects[0].caster->SetIndexData(data, datasize / sizeof(WORD));
	}
	adjbuffer->Unmap();
	adjbuffer->Release();

	objects[1].caster->GetAdjacencyBuffer(&adjbuffer);
	objects[1].caster->Discard(D3DX10_MESH_DISCARD_DEVICE_BUFFERS);

	adjbuffer->Map((void**)&data, &datasize);
	{
		objects[1].caster->SetIndexData(data, datasize / sizeof(WORD));
	}
	adjbuffer->Unmap();
	adjbuffer->Release();

	objects[2].caster->GetAdjacencyBuffer(&adjbuffer);
	objects[2].caster->Discard(D3DX10_MESH_DISCARD_DEVICE_BUFFERS);

	adjbuffer->Map((void**)&data, &datasize);
	{
		objects[2].caster->SetIndexData(data, datasize / sizeof(WORD));
	}
	adjbuffer->Unmap();
	adjbuffer->Release();

	objects[0].caster->CommitToDevice();
	objects[1].caster->CommitToDevice();
	objects[2].caster->CommitToDevice();

	// misc
	D3DXMatrixTranslation(&objects[0].world, -1.5f, 0.55f, 1.2f);
	D3DXMatrixTranslation(&objects[1].world, 1.0f, 0.55f, 0.7f);
	D3DXMatrixTranslation(&objects[2].world, 0, 0.55f, -1);

	objects[0].caster->GetVertexDescription(&decl, &declsize);
	technique2->GetPassByIndex(0)->GetDesc(&passdesc);

	MYVALID(device->CreateInputLayout(decl, declsize, passdesc.pIAInputSignature, passdesc.IAInputSignatureSize, &vertexlayout2));

	cameraangle = D3DXVECTOR2(0.78f, 0.78f);
	lightangle = D3DXVECTOR2(2.8f, 0.78f);

	// blend states
	D3D10_BLEND_DESC blenddesc;

	memset(&blenddesc, 0, sizeof(D3D10_BLEND_DESC));

	blenddesc.BlendEnable[0]			= 1;
	blenddesc.BlendOp					= D3D10_BLEND_OP_ADD;
	blenddesc.BlendOpAlpha				= D3D10_BLEND_OP_ADD;
	blenddesc.DestBlend					= D3D10_BLEND_INV_SRC_ALPHA;
	blenddesc.DestBlendAlpha			= D3D10_BLEND_ZERO;
	blenddesc.RenderTargetWriteMask[0]	= D3D10_COLOR_WRITE_ENABLE_ALL;
	blenddesc.SrcBlend					= D3D10_BLEND_SRC_ALPHA;
	blenddesc.SrcBlendAlpha				= D3D10_BLEND_ONE;

	device->CreateBlendState(&blenddesc, &alphablend);

	blenddesc.BlendEnable[0]			= 0;
	blenddesc.DestBlend					= D3D10_BLEND_ZERO;
	blenddesc.SrcBlend					= D3D10_BLEND_ONE;

	device->CreateBlendState(&blenddesc, &noblend);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	for( int i = 0; i < NUM_OBJECTS; ++i )
	{
		SAFE_RELEASE(objects[i].object);
		SAFE_RELEASE(objects[i].caster);
	}

	SAFE_RELEASE(alphablend);
	SAFE_RELEASE(noblend);

	SAFE_RELEASE(vertexlayout1);
	SAFE_RELEASE(vertexlayout2);
	SAFE_RELEASE(specular);
	SAFE_RELEASE(volume);
	SAFE_RELEASE(receiver);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(texture2);
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
}
//*************************************************************************************************************
void Update(float delta)
{
	D3DXVECTOR2 velocity(mousedx, mousedy);

	cameraangle.prev = cameraangle.curr;
	lightangle.prev = lightangle.curr;

	if( mousedown == 1 )
		cameraangle.curr += velocity * 0.004f;

	if( mousedown == 2 )
		lightangle.curr += velocity * 0.004f;

	// clamp to [-pi, pi]
	if( cameraangle.curr.y >= 1.5f )
		cameraangle.curr.y = 1.5f;

	if( cameraangle.curr.y <= -1.5f )
		cameraangle.curr.y = -1.5f;

	// clamp to [0.1, pi]
	if( lightangle.curr.y >= 1.5f )
		lightangle.curr.y = 1.5f;

	if( lightangle.curr.y <= 0.1f )
		lightangle.curr.y = 0.1f;
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	D3DXMATRIX		view, proj, viewproj;
	D3DXMATRIX		world;
	D3DXMATRIX		inv;

	D3DXVECTOR4		uvscale(3, 3, 0, 0);
	D3DXVECTOR4		volumecolor(1, 1, 0, 0.5f);
	D3DXVECTOR4		lightposos;

	D3DXVECTOR3		lightpos(0, 0, -10);
	D3DXVECTOR3		eye(0, 0, -5.2f);
	D3DXVECTOR3		look(0, 0.5f, 0);
	D3DXVECTOR3		up(0, 1, 0);
	D3DXVECTOR3		p1, p2;
	D3DXVECTOR2		orient	= cameraangle.smooth(alpha);
	D3DXVECTOR2		light	= lightangle.smooth(alpha);

	time += elapsedtime;

	// setup things
	D3DXMatrixRotationYawPitchRoll(&view, light.x, light.y, 0);
	D3DXVec3TransformCoord(&lightpos, &lightpos, &view);

	D3DXMatrixRotationYawPitchRoll(&view, orient.x, orient.y, 0);
	D3DXVec3TransformCoord(&eye, &eye, &view);

	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixPerspectiveFovLH(&proj, (float)D3DX_PI / 4.0f, (float)screenwidth / (float)screenheight, 0.1f, 20);

	proj._33 = 1;
	proj._43 = -0.1f;

	D3DXMatrixMultiply(&viewproj, &view, &proj);
	D3DXMatrixScaling(&world, 5, 0.1f, 5);
	D3DXMatrixInverse(&inv, 0, &world);

	float color[4] = { 0.0f, 0.125f, 0.3f, 1.0f };

	device->ClearRenderTargetView(rendertargetview, color);
	device->ClearDepthStencilView(depthstencilview, D3D10_CLEAR_DEPTH, 1.0f, 0);
	{
		device->IASetInputLayout(vertexlayout1);
		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device->OMSetBlendState(noblend, 0, 0xffffffff);

		// draw scene
		specular->GetVariableByName("basetex")->AsShaderResource()->SetResource(texture2);

		specular->GetVariableByName("matViewProj")->AsMatrix()->SetMatrix((float*)&viewproj);
		specular->GetVariableByName("matWorld")->AsMatrix()->SetMatrix((float*)&world);
		specular->GetVariableByName("matWorldInv")->AsMatrix()->SetMatrix((float*)&inv);

		specular->GetVariableByName("eyePos")->AsVector()->SetFloatVector((float*)&eye);
		specular->GetVariableByName("lightPos")->AsVector()->SetFloatVector((float*)&lightpos);
		specular->GetVariableByName("uvScale")->AsVector()->SetFloatVector((float*)&uvscale);

		technique1->GetPassByIndex(0)->Apply(0);
		receiver->DrawSubset(0);

		uvscale = D3DXVECTOR4(1, 1, 0, 0);

		specular->GetVariableByName("basetex")->AsShaderResource()->SetResource(texture1);
		specular->GetVariableByName("uvScale")->AsVector()->SetFloatVector((float*)&uvscale);

		for( int i = 0; i < NUM_OBJECTS; ++i )
		{
			const ShadowCaster& caster = objects[i];

			D3DXMatrixInverse(&inv, 0, &caster.world);

			specular->GetVariableByName("matWorld")->AsMatrix()->SetMatrix((float*)&caster.world);
			specular->GetVariableByName("matWorldInv")->AsMatrix()->SetMatrix((float*)&inv);

			technique1->GetPassByIndex(0)->Apply(0);
			caster.object->DrawSubset(0);
		}

		// draw shadow volume
		volume->GetVariableByName("matViewProj")->AsMatrix()->SetMatrix((float*)&viewproj);
		volume->GetVariableByName("faceColor")->AsVector()->SetFloatVector((float*)&volumecolor);

		device->IASetInputLayout(vertexlayout2);
		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);

		device->OMSetBlendState(alphablend, 0, 0xffffffff);

		for( int i = 0; i < NUM_OBJECTS; ++i )
		{
			const ShadowCaster& caster = objects[i];

			D3DXMatrixInverse(&inv, 0, &caster.world);
			D3DXVec3Transform(&lightposos, &lightpos, &inv);

			volume->GetVariableByName("matWorld")->AsMatrix()->SetMatrix((float*)&caster.world);
			volume->GetVariableByName("lightPos")->AsVector()->SetFloatVector((float*)&lightposos);

			technique2->GetPassByIndex(0)->Apply(0);
			caster.caster->DrawSubset(0);
		}
	}
	swapchain->Present(0, 0);
}
//*************************************************************************************************************
