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

// external functions
extern HRESULT LoadMeshFromQM(LPCTSTR file, DWORD options, ID3DX10Mesh** mesh);
extern HRESULT RenderText(const std::string& str, ID3D10Texture2D* tex, DWORD width, DWORD height);

// tutorial structures
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
ID3D10Effect*				ambient			= 0;
ID3D10Effect*				specular		= 0;
ID3D10Effect*				volume			= 0;
ID3DX10Mesh*				receiver		= 0;
ID3D10InputLayout*			vertexlayout1	= 0;
ID3D10InputLayout*			vertexlayout2	= 0;
ID3D10InputLayout*			vertexlayout3	= 0;

ID3D10ShaderResourceView*	texture1		= 0;
ID3D10ShaderResourceView*	texture2		= 0;
ID3D10ShaderResourceView*	texture3		= 0;
ID3D10Texture2D*			text			= 0;

ID3D10EffectTechnique*		technique1		= 0;
ID3D10EffectTechnique*		technique2		= 0;
ID3D10EffectTechnique*		technique3		= 0;

ID3D10BlendState*			noblend			= 0;
ID3D10BlendState*			noblend_nocolor	= 0;
ID3D10BlendState*			alphablend		= 0;
ID3D10BlendState*			additive		= 0;
ID3D10DepthStencilState*	findshadow		= 0;
ID3D10DepthStencilState*	maskshadow		= 0;
ID3D10DepthStencilState*	nostencil		= 0;
ID3D10RasterizerState*		nocull			= 0;
ID3D10RasterizerState*		backfacecull	= 0;

ID3DX10Sprite*				spritebatch		= 0;
ShadowCaster				objects[NUM_OBJECTS];
state<D3DXVECTOR2>			cameraangle;
state<D3DXVECTOR2>			lightangle;
bool						drawvolume		= false;

// tutorial functions
void DrawShadowVolume(const D3DXVECTOR3& lightpos);

HRESULT InitScene()
{
	ID3D10Blob*	errors		= NULL;
	UINT		hlslflags	= D3D10_SHADER_ENABLE_STRICTNESS;
	HRESULT		hr;

#ifdef _DEBUG
	hlslflags |= D3D10_SHADER_DEBUG;
#endif

	D3D10_TEXTURE2D_DESC desc;

	desc.ArraySize			= 1;
	desc.BindFlags			= D3D10_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags		= D3D10_CPU_ACCESS_WRITE;
	desc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Height				= 128;
	desc.MipLevels			= 1;
	desc.MiscFlags			= 0;
	desc.SampleDesc.Count	= 1;
	desc.SampleDesc.Quality	= 0;
	desc.Usage				= D3D10_USAGE_DYNAMIC;
	desc.Width				= 512;

	MYVALID(device->CreateTexture2D(&desc, 0, &text));
	RenderText("Use mouse to rotate camera and light\n\n1: draw shadow volume", text, 512, 128);

	D3D10_SHADER_RESOURCE_VIEW_DESC rvdesc;

	rvdesc.Format						= DXGI_FORMAT_R8G8B8A8_UNORM;
	rvdesc.ViewDimension				= D3D10_SRV_DIMENSION_TEXTURE2D;
	rvdesc.Texture2D.MostDetailedMip	= 0;
	rvdesc.Texture2D.MipLevels			= 1;

	MYVALID(device->CreateShaderResourceView(text, &rvdesc, &texture3));
	D3DX10CreateSprite(device, 1, &spritebatch);

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

	hr = D3DX10CreateEffectFromFile("../media/shaders10/ambient10.fx", 0, 0, "fx_4_0", hlslflags, 0, device, 0, 0, &ambient, &errors, 0);

	if( errors )
	{
		char* str = (char*)errors->GetBufferPointer();
		std::cout << str << "\n";
	}

	if( FAILED(hr) )
		return hr;

	technique1 = specular->GetTechniqueByName("specular");
	technique2 = volume->GetTechniqueByName("extrude");
	technique3 = ambient->GetTechniqueByName("ambient");

	D3D10_PASS_DESC					passdesc;
	const D3D10_INPUT_ELEMENT_DESC*	decl = 0;
	UINT							declsize = 0;

	// receiver
	MYVALID(LoadMeshFromQM("../media/meshes10/box.qm", 0, &receiver));

	receiver->GetVertexDescription(&decl, &declsize);
	technique1->GetPassByIndex(0)->GetDesc(&passdesc);

	MYVALID(device->CreateInputLayout(decl, declsize, passdesc.pIAInputSignature, passdesc.IAInputSignatureSize, &vertexlayout1));

	technique3->GetPassByIndex(0)->GetDesc(&passdesc);

	MYVALID(device->CreateInputLayout(decl, declsize, passdesc.pIAInputSignature, passdesc.IAInputSignatureSize, &vertexlayout3));

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

	objects[0].caster->Discard(D3DX10_MESH_DISCARD_DEVICE_BUFFERS);
	objects[1].caster->Discard(D3DX10_MESH_DISCARD_DEVICE_BUFFERS);
	objects[2].caster->Discard(D3DX10_MESH_DISCARD_DEVICE_BUFFERS);

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

	// states
	D3D10_BLEND_DESC			blenddesc;
	D3D10_DEPTH_STENCIL_DESC	depthdesc;
	D3D10_RASTERIZER_DESC		rasterdesc;

	memset(&blenddesc, 0, sizeof(D3D10_BLEND_DESC));
	memset(&depthdesc, 0, sizeof(D3D10_DEPTH_STENCIL_DESC));
	memset(&rasterdesc, 0, sizeof(D3D10_RASTERIZER_DESC));

	blenddesc.BlendEnable[0]			= 1;
	blenddesc.BlendOp					= D3D10_BLEND_OP_ADD;
	blenddesc.BlendOpAlpha				= D3D10_BLEND_OP_ADD;
	blenddesc.DestBlend					= D3D10_BLEND_INV_SRC_ALPHA;
	blenddesc.DestBlendAlpha			= D3D10_BLEND_ZERO;
	blenddesc.RenderTargetWriteMask[0]	= D3D10_COLOR_WRITE_ENABLE_ALL;
	blenddesc.SrcBlend					= D3D10_BLEND_SRC_ALPHA;
	blenddesc.SrcBlendAlpha				= D3D10_BLEND_ONE;

	device->CreateBlendState(&blenddesc, &alphablend);

	blenddesc.DestBlend					= D3D10_BLEND_ONE;
	blenddesc.SrcBlend					= D3D10_BLEND_ONE;

	device->CreateBlendState(&blenddesc, &additive);

	blenddesc.BlendEnable[0]			= 0;
	blenddesc.DestBlend					= D3D10_BLEND_ZERO;
	blenddesc.SrcBlend					= D3D10_BLEND_ONE;

	device->CreateBlendState(&blenddesc, &noblend);

	blenddesc.RenderTargetWriteMask[0]	= 0;
	device->CreateBlendState(&blenddesc, &noblend_nocolor);

	// increment on back face, decrement on front face
	depthdesc.DepthEnable					= TRUE;
	depthdesc.DepthFunc						= D3D10_COMPARISON_LESS;
	depthdesc.DepthWriteMask				= D3D10_DEPTH_WRITE_MASK_ZERO;
	depthdesc.StencilEnable					= TRUE;
	depthdesc.StencilReadMask				= D3D10_DEFAULT_STENCIL_READ_MASK;
	depthdesc.StencilWriteMask				= D3D10_DEFAULT_STENCIL_WRITE_MASK;

	depthdesc.BackFace.StencilFunc			= D3D10_COMPARISON_ALWAYS;
	depthdesc.BackFace.StencilDepthFailOp	= D3D10_STENCIL_OP_INCR;
	depthdesc.BackFace.StencilFailOp		= D3D10_STENCIL_OP_KEEP;
	depthdesc.BackFace.StencilPassOp		= D3D10_STENCIL_OP_KEEP;

	depthdesc.FrontFace.StencilFunc			= D3D10_COMPARISON_ALWAYS;
	depthdesc.FrontFace.StencilDepthFailOp	= D3D10_STENCIL_OP_DECR;
	depthdesc.FrontFace.StencilFailOp		= D3D10_STENCIL_OP_KEEP;
	depthdesc.FrontFace.StencilPassOp		= D3D10_STENCIL_OP_KEEP;

	device->CreateDepthStencilState(&depthdesc, &findshadow);

	// mask out shadow with stencil
	depthdesc.DepthFunc						= D3D10_COMPARISON_LESS_EQUAL;

	depthdesc.FrontFace.StencilDepthFailOp	= D3D10_STENCIL_OP_KEEP;
	depthdesc.FrontFace.StencilFunc			= D3D10_COMPARISON_GREATER;

	depthdesc.BackFace.StencilDepthFailOp	= D3D10_STENCIL_OP_KEEP;
	depthdesc.BackFace.StencilFunc			= D3D10_COMPARISON_ALWAYS;

	device->CreateDepthStencilState(&depthdesc, &maskshadow);

	// reset
	depthdesc.DepthFunc						= D3D10_COMPARISON_LESS;
	depthdesc.DepthWriteMask				= D3D10_DEPTH_WRITE_MASK_ALL;
	depthdesc.StencilEnable					= FALSE;
	depthdesc.FrontFace.StencilFunc			= D3D10_COMPARISON_ALWAYS;

	device->CreateDepthStencilState(&depthdesc, &nostencil);

	rasterdesc.AntialiasedLineEnable	= FALSE;
	rasterdesc.CullMode					= D3D10_CULL_NONE;
	rasterdesc.DepthBias				= 0;
	rasterdesc.DepthBiasClamp			= 0;
	rasterdesc.DepthClipEnable			= TRUE;
	rasterdesc.FillMode					= D3D10_FILL_SOLID;
	rasterdesc.FrontCounterClockwise	= FALSE;
	rasterdesc.MultisampleEnable		= FALSE;
	rasterdesc.ScissorEnable			= FALSE;
	
	device->CreateRasterizerState(&rasterdesc, &nocull);

	rasterdesc.CullMode					= D3D10_CULL_BACK;

	device->CreateRasterizerState(&rasterdesc, &backfacecull);

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
	SAFE_RELEASE(additive);
	SAFE_RELEASE(noblend);
	SAFE_RELEASE(noblend_nocolor);

	SAFE_RELEASE(findshadow);
	SAFE_RELEASE(maskshadow);
	SAFE_RELEASE(nostencil);

	SAFE_RELEASE(backfacecull);
	SAFE_RELEASE(nocull);

	SAFE_RELEASE(vertexlayout1);
	SAFE_RELEASE(vertexlayout2);
	SAFE_RELEASE(vertexlayout3);
	SAFE_RELEASE(ambient);
	SAFE_RELEASE(specular);
	SAFE_RELEASE(volume);
	SAFE_RELEASE(receiver);

	SAFE_RELEASE(spritebatch);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(texture2);
	SAFE_RELEASE(texture3);
	SAFE_RELEASE(text);
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	if( wparam == 0x31 )
		drawvolume = !drawvolume;
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
	D3DXVECTOR4		volumecolor(0, 0, 0, 1);
	D3DXVECTOR4		ambientcolor(0.2f, 0.2f, 0.2f, 1);
	D3DXVECTOR4		lightcolor(0.8f, 0.8f, 0.8f, 1);

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
	device->ClearDepthStencilView(depthstencilview, D3D10_CLEAR_DEPTH|D3D10_CLEAR_STENCIL, 1.0f, 0);
	{
		device->IASetInputLayout(vertexlayout3);
		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// STEP 1: pre-z pass
		ambient->GetVariableByName("matViewProj")->AsMatrix()->SetMatrix((float*)&viewproj);
		ambient->GetVariableByName("matWorld")->AsMatrix()->SetMatrix((float*)&world);
		ambient->GetVariableByName("lightAmbient")->AsVector()->SetFloatVector((float*)&ambientcolor);
		ambient->GetVariableByName("basetex")->AsShaderResource()->SetResource(texture2);
		ambient->GetVariableByName("uvScale")->AsVector()->SetFloatVector((float*)&uvscale);

		technique3->GetPassByIndex(0)->Apply(0);
		receiver->DrawSubset(0);

		uvscale = D3DXVECTOR4(1, 1, 0, 0);

		ambient->GetVariableByName("basetex")->AsShaderResource()->SetResource(texture1);
		ambient->GetVariableByName("uvScale")->AsVector()->SetFloatVector((float*)&uvscale);

		for( int i = 0; i < NUM_OBJECTS; ++i )
		{
			const ShadowCaster& caster = objects[i];

			ambient->GetVariableByName("matWorld")->AsMatrix()->SetMatrix((float*)&caster.world);

			technique3->GetPassByIndex(0)->Apply(0);
			caster.object->DrawSubset(0);
		}

		// STEP 2: Carmack's reverse
		device->OMSetBlendState(noblend_nocolor, 0, 0xffffffff);
		device->OMSetDepthStencilState(findshadow, 0);
		device->RSSetState(nocull);

		volume->GetVariableByName("matViewProj")->AsMatrix()->SetMatrix((float*)&viewproj);
		volume->GetVariableByName("faceColor")->AsVector()->SetFloatVector((float*)&volumecolor);

		DrawShadowVolume(lightpos);

		device->RSSetState(backfacecull);

		// STEP 3: multipass lighting
		uvscale = D3DXVECTOR4(3, 3, 0, 0);

		device->IASetInputLayout(vertexlayout1);
		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		device->OMSetBlendState(additive, 0, 0xffffffff);
		device->OMSetDepthStencilState(maskshadow, 1);

		specular->GetVariableByName("basetex")->AsShaderResource()->SetResource(texture2);

		specular->GetVariableByName("matViewProj")->AsMatrix()->SetMatrix((float*)&viewproj);
		specular->GetVariableByName("matWorld")->AsMatrix()->SetMatrix((float*)&world);
		specular->GetVariableByName("matWorldInv")->AsMatrix()->SetMatrix((float*)&inv);

		specular->GetVariableByName("eyePos")->AsVector()->SetFloatVector((float*)&eye);
		specular->GetVariableByName("lightPos")->AsVector()->SetFloatVector((float*)&lightpos);
		specular->GetVariableByName("lightColor")->AsVector()->SetFloatVector((float*)&lightcolor);
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

		device->OMSetDepthStencilState(nostencil, 0);

		if( drawvolume )
		{
			volumecolor = D3DXVECTOR4(1, 1, 0, 0.5f);
			volume->GetVariableByName("matViewProj")->AsMatrix()->SetMatrix((float*)&viewproj); //
			volume->GetVariableByName("faceColor")->AsVector()->SetFloatVector((float*)&volumecolor);

			device->OMSetBlendState(alphablend, 0, 0xffffffff);
			DrawShadowVolume(lightpos);
		}


		// render text
		D3DX10_SPRITE sprite;

		sprite.ColorModulate	= D3DXCOLOR(1, 1, 1, 1);
		sprite.pTexture			= texture3;
		sprite.TexCoord			= D3DXVECTOR2(0, 0);
		sprite.TexSize			= D3DXVECTOR2(1, 1);
		sprite.TextureIndex		= 0;

		sprite.matWorld = D3DXMATRIX(
			512, 0, 0, 0,
			0, 128, 0, 0,
			0, 0, 1, 0,
			10.0f - (screenwidth - 512) * 0.5f,
			(screenheight - 128) * 0.5f - 10.0f,
			0, 1);

		device->OMSetBlendState(alphablend, 0, 0xffffffff);
		D3DXMatrixOrthoLH(&proj, (float)screenwidth, (float)screenheight, 0, 1);

		spritebatch->SetProjectionTransform(&proj);
		spritebatch->Begin(D3DX10_SPRITE_SAVE_STATE);
		{
			spritebatch->DrawSpritesImmediate(&sprite, 1, 0, 0);
		}
		spritebatch->End();

		device->OMSetBlendState(noblend, 0, 0xffffffff);
	}
	swapchain->Present(0, 0);
}
//*************************************************************************************************************
void DrawShadowVolume(const D3DXVECTOR3& lightpos)
{
	D3DXMATRIX		inv;
	D3DXVECTOR4		lightposos;
	D3DXVECTOR3		offset;

	device->IASetInputLayout(vertexlayout2);
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);

	for( int i = 0; i < NUM_OBJECTS; ++i )
	{
		const ShadowCaster& caster = objects[i];

		D3DXMatrixInverse(&inv, 0, &caster.world);
		D3DXVec3Transform(&lightposos, &lightpos, &inv);

		// assume object center is the origin
		offset = D3DXVECTOR3(-lightposos);
		D3DXVec3Normalize(&offset, &offset);
		offset *= 2e-2f;

		volume->GetVariableByName("matWorld")->AsMatrix()->SetMatrix((float*)&caster.world);
		volume->GetVariableByName("lightPos")->AsVector()->SetFloatVector((float*)&lightposos);
		volume->GetVariableByName("offset")->AsVector()->SetFloatVector((float*)&offset);

		technique2->GetPassByIndex(0)->Apply(0);
		caster.caster->DrawSubset(0);
	}
}
//*************************************************************************************************************
