//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>
#include <set>
#include <vector>

#include "../common/dxext.h"
#include "../common/orderedarray.hpp"

// helper macros
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern LPDIRECT3DDEVICE9 device;

extern long		screenwidth;
extern long		screenheight;
extern short	mousedx;
extern short	mousedy;
extern short	mousedown;

// tutorial structs
struct ShadowVertex
{
	float x, y, z;
};

struct NormalVertex
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};

struct Edge
{
	DWORD i1, i2;				// vertex indices
	D3DXVECTOR3 v1, v2;			// vertex positions

	mutable DWORD other;		// other triangle
	mutable D3DXVECTOR3 n1, n2;	// triangle normals
	mutable bool flip;			// flip volume quad

	Edge()
	{
		i1 = i2 = 0;
		other = 0xffffffff;
		flip = false;
	}

	inline bool operator <(const Edge& other) const {
		return ((i1 < other.i1) || (i1 == other.i1 && i2 < other.i2));
	}
};

// tutorial variables
typedef mystl::orderedarray<Edge> edgeset;
typedef std::vector<Edge> edgelist;

LPD3DXEFFECT					specular		= NULL;
LPD3DXEFFECT					extrude			= NULL;
LPD3DXMESH						shadowreceiver	= NULL;
LPD3DXMESH						shadowcaster	= NULL;		// shadow caster for silhouette determination
LPD3DXMESH						object			= NULL;		// shadow caster for normal rendering
LPDIRECT3DTEXTURE9				texture1		= NULL;
LPDIRECT3DTEXTURE9				texture2		= NULL;
LPDIRECT3DVERTEXBUFFER9			shadowvolume	= NULL;
LPDIRECT3DVERTEXDECLARATION9	shadowdecl		= NULL;

edgeset					casteredges;
edgelist				silhouette;
state<D3DXVECTOR2>		cameraangle;
state<D3DXVECTOR2>		lightangle;

void GenerateEdges(edgeset& out, LPD3DXMESH mesh);
void FindSilhouette(edgelist& out, const edgeset& edges, const D3DXMATRIX& world, const D3DXVECTOR3& lightpos);
void DrawShadowVolume(const edgelist& silhouette, const D3DXMATRIX& world, const D3DXMATRIX& viewproj, const D3DXVECTOR3& lightpos);

HRESULT InitScene()
{
	HRESULT hr;
	
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		D3DDECL_END()
	};

	D3DVERTEXELEMENT9 elem2[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	MYVALID(D3DXLoadMeshFromX("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &shadowreceiver));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2.jpg", &texture2));
	MYVALID(DXCreateEffect("../media/shaders/blinnphong.fx", device, &specular));
	MYVALID(DXCreateEffect("../media/shaders/extrude.fx", device, &extrude));

	//MYVALID(D3DXCreateSphere(device, 0.5f, 30, 30, &shadowcaster, NULL));
	//MYVALID(D3DXCreateSphere(device, 0.5f, 30, 30, &object, NULL));

	MYVALID(D3DXCreateMesh(12, 8, D3DXMESH_MANAGED, elem, device, &shadowcaster));
	MYVALID(D3DXCreateMesh(12, 24, D3DXMESH_MANAGED, elem2, device, &object));

	// first create object
	NormalVertex* vdata2 = 0;
	WORD* idata = 0;
	DWORD* adata = 0;

	object->LockVertexBuffer(D3DLOCK_DISCARD, (void**)&vdata2);
	object->LockIndexBuffer(D3DLOCK_DISCARD, (void**)&idata);
	object->LockAttributeBuffer(D3DLOCK_DISCARD, &adata);

	// Z- face
	vdata2[0].x = -0.5f;	vdata2[1].x = -0.5f;	vdata2[2].x = 0.5f;		vdata2[3].x = 0.5f;
	vdata2[0].y = -0.5f;	vdata2[1].y = 0.5f;		vdata2[2].y = 0.5f;		vdata2[3].y = -0.5f;
	vdata2[0].z = -0.5f;	vdata2[1].z = -0.5f;	vdata2[2].z = -0.5f;	vdata2[3].z = -0.5f;

	vdata2[0].nx = vdata2[1].nx = vdata2[2].nx = vdata2[3].nx = 0;
	vdata2[0].ny = vdata2[1].ny = vdata2[2].ny = vdata2[3].ny = 0;
	vdata2[0].nz = vdata2[1].nz = vdata2[2].nz = vdata2[3].nz = -1;

	vdata2 += 4;

	// X- face
	vdata2[0].x = -0.5f;	vdata2[1].x = -0.5f;	vdata2[2].x = -0.5f;	vdata2[3].x = -0.5f;
	vdata2[0].y = -0.5f;	vdata2[1].y = 0.5f;		vdata2[2].y = 0.5f;		vdata2[3].y = -0.5f;
	vdata2[0].z = 0.5f;		vdata2[1].z = 0.5f;		vdata2[2].z = -0.5f;	vdata2[3].z = -0.5f;

	vdata2[0].nx = vdata2[1].nx = vdata2[2].nx = vdata2[3].nx = -1;
	vdata2[0].ny = vdata2[1].ny = vdata2[2].ny = vdata2[3].ny = 0;
	vdata2[0].nz = vdata2[1].nz = vdata2[2].nz = vdata2[3].nz = 0;

	vdata2 += 4;

	// X+ face
	vdata2[0].x = 0.5f;		vdata2[1].x = 0.5f;		vdata2[2].x = 0.5f;		vdata2[3].x = 0.5f;
	vdata2[0].y = -0.5f;	vdata2[1].y = 0.5f;		vdata2[2].y = 0.5f;		vdata2[3].y = -0.5f;
	vdata2[0].z = -0.5f;	vdata2[1].z = -0.5f;	vdata2[2].z = 0.5f;		vdata2[3].z = 0.5f;

	vdata2[0].nx = vdata2[1].nx = vdata2[2].nx = vdata2[3].nx = 1;
	vdata2[0].ny = vdata2[1].ny = vdata2[2].ny = vdata2[3].ny = 0;
	vdata2[0].nz = vdata2[1].nz = vdata2[2].nz = vdata2[3].nz = 0;

	vdata2 += 4;

	// Z+ face
	vdata2[0].x = 0.5f;		vdata2[1].x = 0.5f;		vdata2[2].x = -0.5f;	vdata2[3].x = -0.5f;
	vdata2[0].y = -0.5f;	vdata2[1].y = 0.5f;		vdata2[2].y = 0.5f;		vdata2[3].y = -0.5f;
	vdata2[0].z = 0.5f;		vdata2[1].z = 0.5f;		vdata2[2].z = 0.5f;		vdata2[3].z = 0.5f;

	vdata2[0].nx = vdata2[1].nx = vdata2[2].nx = vdata2[3].nx = 0;
	vdata2[0].ny = vdata2[1].ny = vdata2[2].ny = vdata2[3].ny = 0;
	vdata2[0].nz = vdata2[1].nz = vdata2[2].nz = vdata2[3].nz = 1;

	vdata2 += 4;

	// Y+ face
	vdata2[0].x = -0.5f;	vdata2[1].x = -0.5f;	vdata2[2].x = 0.5f;		vdata2[3].x = 0.5f;
	vdata2[0].y = 0.5f;		vdata2[1].y = 0.5f;		vdata2[2].y = 0.5f;		vdata2[3].y = 0.5f;
	vdata2[0].z = -0.5f;	vdata2[1].z = 0.5f;		vdata2[2].z = 0.5f;		vdata2[3].z = -0.5f;

	vdata2[0].nx = vdata2[1].nx = vdata2[2].nx = vdata2[3].nx = 0;
	vdata2[0].ny = vdata2[1].ny = vdata2[2].ny = vdata2[3].ny = 0;
	vdata2[0].nz = vdata2[1].nz = vdata2[2].nz = vdata2[3].nz = 1;

	vdata2 += 4;

	// Y- face
	vdata2[0].x = -0.5f;	vdata2[1].x = -0.5f;	vdata2[2].x = 0.5f;		vdata2[3].x = 0.5f;
	vdata2[0].y = -0.5f;	vdata2[1].y = -0.5f;	vdata2[2].y = -0.5f;	vdata2[3].y = -0.5f;
	vdata2[0].z = 0.5f;		vdata2[1].z = -0.5f;	vdata2[2].z = -0.5f;	vdata2[3].z = 0.5f;

	vdata2[0].nx = vdata2[1].nx = vdata2[2].nx = vdata2[3].nx = 0;
	vdata2[0].ny = vdata2[1].ny = vdata2[2].ny = vdata2[3].ny = 0;
	vdata2[0].nz = vdata2[1].nz = vdata2[2].nz = vdata2[3].nz = 1;

	for( int i = 0; i < 6; ++i )
	{
		idata[0] = idata[3] = i * 4;
		idata[1] = i * 4 + 1;
		idata[2] = idata[4] = i * 4 + 2;
		idata[5] = i * 4 + 3;

		idata += 6;
	}

	memset(adata, 0, 12 * sizeof(DWORD));

	object->UnlockAttributeBuffer();
	object->UnlockIndexBuffer();
	object->UnlockVertexBuffer();

	// then shadow object
	ShadowVertex* vdata = 0;

	shadowcaster->LockVertexBuffer(D3DLOCK_DISCARD, (void**)&vdata);
	shadowcaster->LockIndexBuffer(D3DLOCK_DISCARD, (void**)&idata);
	shadowcaster->LockAttributeBuffer(D3DLOCK_DISCARD, &adata);

	vdata[0].x = -0.5f;		vdata[1].x = -0.5f;		vdata[2].x = -0.5f;		vdata[3].x = -0.5f;
	vdata[0].y = -0.5f;		vdata[1].y = -0.5f;		vdata[2].y = 0.5f;		vdata[3].y = 0.5f;
	vdata[0].z = -0.5f;		vdata[1].z = 0.5f;		vdata[2].z = -0.5f;		vdata[3].z = 0.5f;

	vdata[4].x = 0.5f;		vdata[5].x = 0.5f;		vdata[6].x = 0.5f;		vdata[7].x = 0.5f;
	vdata[4].y = -0.5f;		vdata[5].y = -0.5f;		vdata[6].y = 0.5f;		vdata[7].y = 0.5f;
	vdata[4].z = -0.5f;		vdata[5].z = 0.5f;		vdata[6].z = -0.5f;		vdata[7].z = 0.5f;

	// you dont have to understand...
	idata[0] = idata[3] = idata[11] = idata[31]					= 0;
	idata[1] = idata[8] = idata[10] = idata[24] = idata[27]		= 2;
	idata[2] = idata[4] = idata[13] = idata[29]					= 6;
	idata[5] = idata[12] = idata[15] = idata[32] = idata[34]	= 4;
	idata[6] = idata[9] = idata[23] = idata[30] = idata[33]		= 1;
	idata[7] = idata[20] = idata[22] = idata[25]				= 3;
	idata[14] = idata[16] = idata[19] = idata[26] = idata[28]	= 7;
	idata[17] = idata[18] = idata[21] = idata[35]				= 5;

	memset(adata, 0, 12 * sizeof(DWORD));

	shadowcaster->UnlockAttributeBuffer();
	shadowcaster->UnlockIndexBuffer();
	shadowcaster->UnlockVertexBuffer();

	// generate edges
	std::cout << "Generating edge info...\n";
	GenerateEdges(casteredges, shadowcaster);

	// shadow volume buffer
	elem[0].Type = D3DDECLTYPE_FLOAT4;

	MYVALID(device->CreateVertexBuffer(casteredges.size() * 6 * sizeof(D3DXVECTOR4), D3DUSAGE_DYNAMIC, D3DFVF_XYZW, D3DPOOL_DEFAULT, &shadowvolume, NULL));
	MYVALID(device->CreateVertexDeclaration(elem, &shadowdecl));

	// effect
	specular->SetFloat("ambient", 0);

	cameraangle = D3DXVECTOR2(0.78f, 0.78f);
	lightangle = D3DXVECTOR2(2.8f, 0.78f);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_RELEASE(shadowdecl);
	SAFE_RELEASE(shadowvolume);
	SAFE_RELEASE(shadowreceiver);
	SAFE_RELEASE(shadowcaster);
	SAFE_RELEASE(object);
	SAFE_RELEASE(specular);
	SAFE_RELEASE(extrude);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(texture2);

	casteredges.clear();
	silhouette.clear();

	silhouette.swap(edgelist());
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

	D3DXMATRIX		view, proj, vp;
	D3DXMATRIX		world[2];
	D3DXMATRIX		inv;
	D3DXVECTOR2		orient	= cameraangle.smooth(alpha);
	D3DXVECTOR2		light	= lightangle.smooth(alpha);
	D3DXVECTOR3		lightpos(0, 0, -3);
	D3DXVECTOR3		eye(0, 0, -5.2f);
	D3DXVECTOR3		look(0, 0.5f, 0);
	D3DXVECTOR3		up(0, 1, 0);
	D3DXVECTOR3		p1, p2;

	// setup light
	D3DXMatrixRotationYawPitchRoll(&view, light.x, light.y, 0);
	D3DXVec3TransformCoord(&lightpos, &lightpos, &view);

	// setup camera
	D3DXMatrixRotationYawPitchRoll(&view, orient.x, orient.y, 0);
	D3DXVec3TransformCoord(&eye, &eye, &view);

	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)screenwidth / (float)screenheight, 0.1f, 100);

	// put farplane to infinity
	proj._33 = 1;
	proj._43 = -0.1f;

	D3DXMatrixMultiply(&vp, &view, &proj);

	// setup world matrices
	D3DXMatrixScaling(&world[1], 5, 0.1f, 5);
	D3DXMatrixScaling(&world[0], 1, 1, 1);

	world[0]._41 = -0.7f;
	world[0]._42 = 0.55f;
	world[0]._43 = 0.7f;

	time += elapsedtime;

	// specular effect uniforms
	specular->SetMatrix("matViewProj", &vp);
	specular->SetVector("eyePos", (D3DXVECTOR4*)&eye);
	specular->SetVector("lightPos", (D3DXVECTOR4*)&lightpos);

	if( SUCCEEDED(device->BeginScene()) )
	{
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0xff6694ed, 1.0f, 0);

		// receiver
		D3DXMatrixInverse(&inv, NULL, &world[1]);

		specular->SetMatrix("matWorld", &world[1]);
		specular->SetMatrix("matWorldInv", &inv);

		device->SetTexture(0, texture2);

		specular->Begin(NULL, 0);
		specular->BeginPass(0);
		{
			shadowreceiver->DrawSubset(0);
		}
		specular->EndPass();
		specular->End();

		// caster
		D3DXMatrixInverse(&inv, NULL, &world[0]);

		specular->SetMatrix("matWorld", &world[0]);
		specular->SetMatrix("matWorldInv", &inv);

		device->SetTexture(0, texture1);

		specular->Begin(NULL, 0);
		specular->BeginPass(0);
		{
			object->DrawSubset(0);
		}
		specular->EndPass();
		specular->End();

		device->SetTexture(0, 0);

		// draw shadow with depth fail method
		device->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCRSAT);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

		FindSilhouette(silhouette, casteredges, world[0], (D3DXVECTOR3&)lightpos);
		DrawShadowVolume(silhouette, world[0], vp, lightpos);

		device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_DECRSAT);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

		DrawShadowVolume(silhouette, world[0], vp, lightpos);

		device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA);
		
		// TODO: a quad nem jo
		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL);
		device->SetRenderState(D3DRS_STENCILREF, 1);
		device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		
		struct {
			float x, y, z, w;
			DWORD color;
		} quad[] =
		{
			0, 0, 0, 1, 0xaa000000,
			(float)screenwidth, 0, 0, 1, 0xaa000000,
			0, (float)screenheight, 0, 1, 0xaa000000,

			0, (float)screenheight, 0, 1, 0xaa000000,
			(float)screenwidth, 0, 0, 1, 0xaa000000,
			(float)screenwidth, (float)screenheight, 0, 1, 0xaa000000
		};

		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quad, 5 * sizeof(float));

		device->SetRenderState(D3DRS_ZENABLE, TRUE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
void GenerateEdges(edgeset& out, LPD3DXMESH mesh)
{
	bool		is32bit = (mesh->GetOptions() & D3DXMESH_32BIT);
	DWORD		numindices = mesh->GetNumFaces() * 3;
	DWORD		stride = mesh->GetNumBytesPerVertex();
	BYTE*		vdata = 0;
	WORD*		idata = 0;
	WORD		i1, i2, i3;
	Edge		e;
	D3DXVECTOR3	p1, p2, p3;
	D3DXVECTOR3	a, b, n;
	size_t		ind;

	out.clear();

	if( is32bit )
	{
		MYERROR("GenerateEdges(): TODO: 4 byte indices");
		return;
	}

	// generate edge info
	mesh->LockIndexBuffer(D3DLOCK_READONLY, (LPVOID*)&idata);
	mesh->LockVertexBuffer(D3DLOCK_READONLY, (LPVOID*)&vdata);

	out.reserve(512);

	for( DWORD i = 0; i < numindices; i += 3 )
	{
		if( out.capacity() <= out.size() )
			out.reserve(out.size() + 1024);

		i1 = idata[i + 0];
		i2 = idata[i + 1];
		i3 = idata[i + 2];

		p1 = *((D3DXVECTOR3*)(vdata + i1 * stride));
		p2 = *((D3DXVECTOR3*)(vdata + i2 * stride));
		p3 = *((D3DXVECTOR3*)(vdata + i3 * stride));

		a = p2 - p1;
		b = p3 - p1;

		D3DXVec3Cross(&n, &a, &b);
		D3DXVec3Normalize(&n, &n);

		if( i1 < i2 )
		{
			e.i1 = i1;
			e.i2 = i2;
			e.v1 = p1;
			e.v2 = p2;
			e.n1 = n;

			if( !out.insert(e) )
				std::cout << "Crack in object (first triangle)\n";
		}

		if( i2 < i3 )
		{
			e.i1 = i2;
			e.i2 = i3;
			e.v1 = p2;
			e.v2 = p3;
			e.n1 = n;

			if( !out.insert(e) )
				std::cout << "Crack in object (first triangle)\n";
		}

		if( i3 < i1 )
		{
			e.i1 = i3;
			e.i2 = i1;
			e.v1 = p3;
			e.v2 = p1;
			e.n1 = n;

			if( !out.insert(e) )
				std::cout << "Crack in object (first triangle)\n";
		}
	}

	// find second triangle for each edge
	for( DWORD i = 0; i < numindices; i += 3 )
	{
		i1 = idata[i + 0];
		i2 = idata[i + 1];
		i3 = idata[i + 2];

		p1 = *((D3DXVECTOR3*)(vdata + i1 * stride));
		p2 = *((D3DXVECTOR3*)(vdata + i2 * stride));
		p3 = *((D3DXVECTOR3*)(vdata + i3 * stride));

		a = p2 - p1;
		b = p3 - p1;

		D3DXVec3Cross(&n, &a, &b);
		D3DXVec3Normalize(&n, &n);

		if( i1 > i2 )
		{
			e.i1 = i2;
			e.i2 = i1;

			ind = out.find(e);

			if( ind == edgeset::npos )
			{
				std::cout << "Lone triangle\n";
				continue;
			}

			if( out[ind].other == 0xffffffff )
			{
				out[ind].other = i / 3;
				out[ind].n2 = n;
			}
			else
				std::cout << "Crack in object (second triangle)\n";
		}

		if( i2 > i3 )
		{
			e.i1 = i3;
			e.i2 = i2;

			ind = out.find(e);

			if( ind == edgeset::npos )
			{
				std::cout << "Lone triangle\n";
				continue;
			}

			if( out[ind].other == 0xffffffff )
			{
				out[ind].other = i / 3;
				out[ind].n2 = n;
			}
			else
				std::cout << "Crack in object (second triangle)\n";
		}

		if( i3 > i1 )
		{
			e.i1 = i1;
			e.i2 = i3;

			ind = out.find(e);

			if( ind == edgeset::npos )
			{
				std::cout << "Lone triangle\n";
				continue;
			}

			if( out[ind].other == 0xffffffff )
			{
				out[ind].other = i / 3;
				out[ind].n2 = n;
			}
			else
				std::cout << "Crack in object (second triangle)\n";
		}
	}

	mesh->UnlockIndexBuffer();
	mesh->UnlockVertexBuffer();
}
//*************************************************************************************************************
void FindSilhouette(edgelist& out, const edgeset& edges, const D3DXMATRIX& world, const D3DXVECTOR3& lightpos)
{
	D3DXVECTOR3 p1, n1, n2;
	D3DXMATRIX invt;
	float a, b;

	D3DXMatrixInverse(&invt, NULL, &world);
	D3DXMatrixTranspose(&invt, &invt);

	out.clear();
	out.reserve(50);

	for( size_t i = 0; i < edges.size(); ++i )
	{
		const Edge& e = edges[i];

		if( e.other != 0xffffffff )
		{
			D3DXVec3TransformCoord(&p1, &e.v1, &world);
			D3DXVec3TransformNormal(&n1, &e.n1, &invt);
			D3DXVec3TransformNormal(&n2, &e.n2, &invt);

			a = D3DXVec3Dot(&lightpos, &n1) - D3DXVec3Dot(&n1, &p1);
			b = D3DXVec3Dot(&lightpos, &n2) - D3DXVec3Dot(&n2, &p1);

			if( (a < 0) != (b < 0) )
			{
				if( out.capacity() <= out.size() )
					out.reserve(out.size() + 50);

				e.flip = (b < 0);
				out.push_back(e);
			}
		}
	}
}
//*************************************************************************************************************
void DrawShadowVolume(const edgelist& silhouette, const D3DXMATRIX& world, const D3DXMATRIX& viewproj, const D3DXVECTOR3& lightpos)
{
	D3DXVECTOR4* vdata = 0;
	D3DXMATRIX inv;
	D3DXVECTOR3 lp;

	shadowvolume->Lock(0, 0, (void**)&vdata, D3DLOCK_DISCARD);

	D3DXMatrixInverse(&inv, NULL, &world);
	D3DXVec3TransformCoord(&lp, &lightpos, &inv);

	for( size_t i = 0; i < silhouette.size() * 6; i += 6 )
	{
		const Edge& e = silhouette[i / 6];

		if( e.flip )
		{
			vdata[i] = D3DXVECTOR4(e.v1, 1);
			vdata[i + 1] = D3DXVECTOR4(e.v1 - lp, 0);
			vdata[i + 2] = D3DXVECTOR4(e.v2, 1);

			vdata[i + 3] = D3DXVECTOR4(e.v2, 1);
			vdata[i + 4] = D3DXVECTOR4(e.v1 - lp, 0);
			vdata[i + 5] = D3DXVECTOR4(e.v2 - lp, 0);
		}
		else
		{
			vdata[i] = D3DXVECTOR4(e.v1, 1);
			vdata[i + 1] = D3DXVECTOR4(e.v2, 1);
			vdata[i + 2] = D3DXVECTOR4(e.v1 - lp, 0);

			vdata[i + 3] = D3DXVECTOR4(e.v2, 1);
			vdata[i + 4] = D3DXVECTOR4(e.v2 - lp, 0);
			vdata[i + 5] = D3DXVECTOR4(e.v1 - lp, 0);
		}
	}

	shadowvolume->Unlock();

	extrude->SetMatrix("matWorld", &world);
	extrude->SetMatrix("matViewProj", &viewproj);

	extrude->Begin(0, 0);
	extrude->BeginPass(0);
	{
		device->SetVertexDeclaration(shadowdecl);
		device->SetStreamSource(0, shadowvolume, 0, sizeof(D3DXVECTOR4));
		device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, silhouette.size() * 2);
	}
	extrude->EndPass();
	extrude->End();
}
//*************************************************************************************************************
