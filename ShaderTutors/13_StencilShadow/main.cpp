//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>
#include <set>
#include <vector>

#include "../common/dxext.h"
#include "../common/orderedarray.hpp"

// TODO:
// - optim
// - ambient light
// - UI: draw silhouette

#define NUM_OBJECTS			3
#define MAX_NUM_EDGES		1024	// 4096 vert, 64 KB
#define VOLUME_OFFSET		2e-2f	// to avoid zfight

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
struct Edge
{
	// orderedarray returns read-only reference
	mutable D3DXVECTOR3	v1, v2;		// vertex positions
	mutable D3DXVECTOR3	n1, n2;		// triangle normals
	DWORD				i1, i2;		// vertex indices
	mutable DWORD		other;		// other triangle

	Edge()
	{
		i1 = i2 = 0;
		other = 0xffffffff;
	}

	inline bool operator <(const Edge& other) const {
		return ((i1 < other.i1) || (i1 == other.i1 && i2 < other.i2));
	}
};

typedef mystl::orderedarray<Edge> edgeset;
typedef std::vector<Edge> edgelist;

struct ShadowCaster
{
	D3DXMATRIX	world;
	edgeset		edges;
	edgelist	silhouette;
	LPD3DXMESH	object;		// for normal rendering
	LPD3DXMESH	caster;		// for silhouette determination
};

// tutorial variables
LPD3DXEFFECT					specular		= NULL;
LPD3DXEFFECT					extrude			= NULL;
LPD3DXMESH						shadowreceiver	= NULL;
LPDIRECT3DTEXTURE9				texture1		= NULL;
LPDIRECT3DTEXTURE9				texture2		= NULL;
LPDIRECT3DVERTEXBUFFER9			shadowvolume	= NULL;
LPDIRECT3DINDEXBUFFER9			shadowindices	= NULL;
LPDIRECT3DVERTEXDECLARATION9	shadowdecl		= NULL;

state<D3DXVECTOR2>				cameraangle;
state<D3DXVECTOR2>				lightangle;

ShadowCaster objects[NUM_OBJECTS]; // leak

// tutorial functions
void GenerateEdges(edgeset& out, LPD3DXMESH mesh);
void FindSilhouette(ShadowCaster& caster, const D3DXVECTOR3& lightpos);
void DrawShadowVolume(const ShadowCaster& caster, const D3DXMATRIX& viewproj, const D3DXVECTOR3& lightpos);

HRESULT InitScene()
{
	HRESULT hr;

	MYVALID(D3DXLoadMeshFromX("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &shadowreceiver)); //
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2.jpg", &texture2));
	MYVALID(DXCreateEffect("../media/shaders/blinnphong.fx", device, &specular));
	MYVALID(DXCreateEffect("../media/shaders/extrude.fx", device, &extrude));
	
	// box
	MYVALID(DXCreateTexturedBox(device, D3DXMESH_MANAGED, &objects[0].object));
	MYVALID(DXCreateCollisionBox(device, D3DXMESH_SYSTEMMEM, &objects[0].caster));

	D3DXMatrixTranslation(&objects[0].world, -1.5f, 0.55f, 1.2f);

	// sphere
	MYVALID(D3DXCreateSphere(device, 0.5f, 30, 30, &objects[1].object, NULL));
	MYVALID(DXCreateCollisionSphere(device, 0.5f, 30, 30, D3DXMESH_SYSTEMMEM, &objects[1].caster));

	D3DXMatrixTranslation(&objects[1].world, 1.0f, 0.55f, 0.7f);

	// L shape
	MYVALID(DXCreateTexturedLShape(device, D3DXMESH_MANAGED, &objects[2].object));
	MYVALID(DXCreateCollisionLShape(device, D3DXMESH_SYSTEMMEM, &objects[2].caster));

	D3DXMatrixTranslation(&objects[2].world, 0, 0.55f, -1);

	// generate edges
	std::cout << "Generating edge info...\n";

	for( int i = 0; i < NUM_OBJECTS; ++i )
		GenerateEdges(objects[i].edges, objects[i].caster);

	// shadow volume buffer
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		D3DDECL_END()
	};

	MYVALID(device->CreateIndexBuffer(MAX_NUM_EDGES * 6 * sizeof(WORD), D3DUSAGE_DYNAMIC, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &shadowindices, NULL));
	MYVALID(device->CreateVertexBuffer(MAX_NUM_EDGES * 4 * sizeof(D3DXVECTOR4), D3DUSAGE_DYNAMIC, D3DFVF_XYZW, D3DPOOL_DEFAULT, &shadowvolume, NULL));
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
	for( int i = 0; i < NUM_OBJECTS; ++i )
	{
		SAFE_RELEASE(objects[i].object);
		SAFE_RELEASE(objects[i].caster);

		objects[i].edges.clear();
		objects[i].edges.swap(edgeset());

		objects[i].silhouette.clear();
		objects[i].silhouette.swap(edgelist());
	}

	SAFE_RELEASE(shadowdecl);
	SAFE_RELEASE(shadowvolume);
	SAFE_RELEASE(shadowindices);
	SAFE_RELEASE(shadowreceiver);
	SAFE_RELEASE(specular);
	SAFE_RELEASE(extrude);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(texture2);
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
	D3DXMATRIX		world;
	D3DXMATRIX		inv;
	D3DXVECTOR4		uv(3, 3, 0, 0);
	D3DXVECTOR3		lightpos(0, 0, -10);
	D3DXVECTOR3		eye(0, 0, -5.2f);
	D3DXVECTOR3		look(0, 0.5f, 0);
	D3DXVECTOR3		up(0, 1, 0);
	D3DXVECTOR3		p1, p2;
	D3DXVECTOR2		orient	= cameraangle.smooth(alpha);
	D3DXVECTOR2		light	= lightangle.smooth(alpha);

	// setup light
	D3DXMatrixRotationYawPitchRoll(&view, light.x, light.y, 0);
	D3DXVec3TransformCoord(&lightpos, &lightpos, &view);

	// TODO: no need to calculate every frame
	for( int i = 0; i < NUM_OBJECTS; ++i )
		FindSilhouette(objects[i], (D3DXVECTOR3&)lightpos);

	// setup camera
	D3DXMatrixRotationYawPitchRoll(&view, orient.x, orient.y, 0);
	D3DXVec3TransformCoord(&eye, &eye, &view);

	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)screenwidth / (float)screenheight, 0.1f, 20);

	// put far plane to infinity
	proj._33 = 1;
	proj._43 = -0.1f;

	D3DXMatrixMultiply(&vp, &view, &proj);
	D3DXMatrixScaling(&world, 5, 0.1f, 5);

	time += elapsedtime;

	// specular effect uniforms
	specular->SetMatrix("matViewProj", &vp);
	specular->SetVector("eyePos", (D3DXVECTOR4*)&eye);
	specular->SetVector("lightPos", (D3DXVECTOR4*)&lightpos);

	if( SUCCEEDED(device->BeginScene()) )
	{
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0xff6694ed, 1.0f, 0);

		// STEP 1: z pass
		extrude->SetTechnique("zpass");
		extrude->SetMatrix("matWorld", &world);
		extrude->SetMatrix("matViewProj", &vp);

		extrude->Begin(0, 0);
		extrude->BeginPass(0);
		{
			shadowreceiver->DrawSubset(0);

			for( int i = 0; i < NUM_OBJECTS; ++i )
			{
				extrude->SetMatrix("matWorld", &objects[i].world);
				extrude->CommitChanges();

				objects[i].object->DrawSubset(0);
			}
		}
		extrude->EndPass();
		extrude->End();

		// STEP 2: draw shadow with depth fail method
		device->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCRSAT);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

		for( int i = 0; i < NUM_OBJECTS; ++i )
			DrawShadowVolume(objects[i], vp, lightpos);

		device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_DECRSAT);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

		for( int i = 0; i < NUM_OBJECTS; ++i )
			DrawShadowVolume(objects[i], vp, lightpos);

		device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA);

		// STEP 3: multipass lighting
		device->SetRenderState(D3DRS_ZENABLE, TRUE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_GREATER);
		device->SetRenderState(D3DRS_STENCILREF, 1);

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

		// draw receiver
		D3DXMatrixInverse(&inv, NULL, &world);

		specular->SetMatrix("matWorld", &world);
		specular->SetMatrix("matWorldInv", &inv);
		specular->SetVector("uv", &uv);

		device->SetTexture(0, texture2);

		specular->Begin(NULL, 0);
		specular->BeginPass(0);
		{
			shadowreceiver->DrawSubset(0);
		}
		specular->EndPass();
		specular->End();

		// draw casters
		device->SetTexture(0, texture1);

		specular->Begin(NULL, 0);
		specular->BeginPass(0);
		{
			for( int i = 0; i < NUM_OBJECTS; ++i )
			{
				D3DXMatrixInverse(&inv, NULL, &objects[i].world);

				specular->SetMatrix("matWorld", &objects[i].world);
				specular->SetMatrix("matWorldInv", &inv);
				specular->CommitChanges();

				objects[i].object->DrawSubset(0);
			}
		}
		specular->EndPass();
		specular->End();

		device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		/*
		device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

		for( int i = 0; i < NUM_OBJECTS; ++i )
			DrawShadowVolume(objects[i], vp, lightpos);

		device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		*/

		device->SetTexture(0, 0);
		device->EndScene();
	}

	device->Present(NULL, NULL, NULL, NULL);
}
//*************************************************************************************************************
void GenerateEdges(edgeset& out, LPD3DXMESH mesh)
{
	D3DXVECTOR3	p1, p2, p3;
	D3DXVECTOR3	a, b, n;
	Edge		e;
	BYTE*		vdata		= 0;
	WORD*		idata		= 0;
	size_t		ind;
	DWORD		numindices	= mesh->GetNumFaces() * 3;
	DWORD		stride		= mesh->GetNumBytesPerVertex();
	WORD		i1, i2, i3;
	bool		is32bit		= (mesh->GetOptions() & D3DXMESH_32BIT);

	out.clear();

	if( is32bit )
	{
		MYERROR("GenerateEdges(): 4 byte indices not implemented yet");
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
				std::cout << "Crack in mesh (first triangle)\n";
		}

		if( i2 < i3 )
		{
			e.i1 = i2;
			e.i2 = i3;
			e.v1 = p2;
			e.v2 = p3;
			e.n1 = n;

			if( !out.insert(e) )
				std::cout << "Crack in mesh (first triangle)\n";
		}

		if( i3 < i1 )
		{
			e.i1 = i3;
			e.i2 = i1;
			e.v1 = p3;
			e.v2 = p1;
			e.n1 = n;

			if( !out.insert(e) )
				std::cout << "Crack in mesh (first triangle)\n";
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
				std::cout << "Crack in mesh (second triangle)\n";
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
				std::cout << "Crack in mesh (second triangle)\n";
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
				std::cout << "Crack in mesh (second triangle)\n";
		}
	}

	mesh->UnlockIndexBuffer();
	mesh->UnlockVertexBuffer();
}
//*************************************************************************************************************
void FindSilhouette(ShadowCaster& caster, const D3DXVECTOR3& lightpos)
{
	D3DXMATRIX	inv;
	D3DXVECTOR3	lp;
	float		a, b;

	D3DXMatrixInverse(&inv, NULL, &caster.world);
	D3DXVec3TransformCoord(&lp, &lightpos, &inv);

	caster.silhouette.clear();
	caster.silhouette.reserve(50);

	for( size_t i = 0; i < caster.edges.size(); ++i )
	{
		const Edge& e = caster.edges[i];

		if( e.other != 0xffffffff )
		{
			a = D3DXVec3Dot(&lp, &e.n1) - D3DXVec3Dot(&e.n1, &e.v1);
			b = D3DXVec3Dot(&lp, &e.n2) - D3DXVec3Dot(&e.n2, &e.v1);

			if( (a < 0) != (b < 0) )
			{
				if( caster.silhouette.capacity() <= caster.silhouette.size() )
					caster.silhouette.reserve(caster.silhouette.size() + 50);

				if( a < 0 )
				{
					std::swap(e.v1, e.v2);
					std::swap(e.n1, e.n2);
				}

				caster.silhouette.push_back(e);
			}
		}
	}
}
//*************************************************************************************************************
void DrawShadowVolume(const ShadowCaster& caster, const D3DXMATRIX& viewproj, const D3DXVECTOR3& lightpos)
{
	D3DXVECTOR4*	vdata = 0;
	WORD*			idata = 0;
	D3DXMATRIX		inv;
	D3DXVECTOR3		lp;
	D3DXVECTOR3		center(0, 0, 0);
	D3DXVECTOR3		ltocenter;
	float			weight = 0.5f / caster.silhouette.size();

	shadowvolume->Lock(0, 0, (void**)&vdata, D3DLOCK_DISCARD);
	shadowindices->Lock(0, 0, (void**)&idata, D3DLOCK_DISCARD);

	D3DXMatrixInverse(&inv, NULL, &caster.world);
	D3DXVec3TransformCoord(&lp, &lightpos, &inv);

	for( size_t i = 0; i < caster.silhouette.size(); ++i )
	{
		const Edge& e = caster.silhouette[i];

		center += e.v1 * weight;
		center += e.v2 * weight;
	}

	ltocenter = center - lp;
	D3DXVec3Normalize(&ltocenter, &ltocenter);
	ltocenter *= VOLUME_OFFSET;

	// TODO: test if theres enough room (MAX_NUM_EDGES * 6, MAX_NUM_EDGES * 4)

	for( size_t i = 0; i < caster.silhouette.size(); ++i )
	{
		const Edge& e = caster.silhouette[i];

		vdata[i * 4 + 0] = D3DXVECTOR4(e.v1 + ltocenter, 1);
		vdata[i * 4 + 1] = D3DXVECTOR4(e.v1 - lp, 0);
		vdata[i * 4 + 2] = D3DXVECTOR4(e.v2 + ltocenter, 1);
		vdata[i * 4 + 3] = D3DXVECTOR4(e.v2 - lp, 0);

		idata[i * 6 + 0] = i * 4 + 0;
		idata[i * 6 + 1] = i * 4 + 1;
		idata[i * 6 + 2] = i * 4 + 2;

		idata[i * 6 + 3] = i * 4 + 2;
		idata[i * 6 + 4] = i * 4 + 1;
		idata[i * 6 + 5] = i * 4 + 3;
	}

	// back cap
	vdata[caster.silhouette.size() * 4] = D3DXVECTOR4(center - lp, 0);
	idata += caster.silhouette.size() * 6;

	for( size_t i = 0; i < caster.silhouette.size(); ++i )
	{
		const Edge& e = caster.silhouette[i];

		idata[i * 3 + 0] = i * 4 + 3;
		idata[i * 3 + 1] = i * 4 + 1;
		idata[i * 3 + 2] = caster.silhouette.size() * 4;
	}

	// front cap
	WORD*			idata2			= NULL;
	D3DXVECTOR3*	vdata2			= NULL;
	WORD			verticesadded	= caster.silhouette.size() * 4 + 1;
	DWORD			indicesadded	= caster.silhouette.size() * 9;
	WORD			i1, i2, i3;
	D3DXVECTOR3		*v1, *v2, *v3;
	D3DXVECTOR3		a, b, n;
	float			dist;

	vdata += caster.silhouette.size() * 4 + 1;
	idata += caster.silhouette.size() * 3;

	caster.caster->LockVertexBuffer(D3DLOCK_READONLY, (void**)&vdata2);
	caster.caster->LockIndexBuffer(D3DLOCK_READONLY, (void**)&idata2);

	for( DWORD i = 0; i < caster.caster->GetNumFaces() * 3; i += 3 )
	{
		i1 = idata2[i + 0];
		i2 = idata2[i + 1];
		i3 = idata2[i + 2];

		v1 = (vdata2 + i1);
		v2 = (vdata2 + i2);
		v3 = (vdata2 + i3);

		a = v2->operator -(*v1);
		b = v3->operator -(*v1);

		D3DXVec3Cross(&n, &a, &b);
		dist = D3DXVec3Dot(&n, &lp) - D3DXVec3Dot(&n, v1);

		if( dist > 0 )
		{
			// faces light, add it
			vdata[0] = D3DXVECTOR4(*v1 + ltocenter, 1);
			vdata[1] = D3DXVECTOR4(*v2 + ltocenter, 1);
			vdata[2] = D3DXVECTOR4(*v3 + ltocenter, 1);

			idata[0] = verticesadded;
			idata[1] = verticesadded + 1;
			idata[2] = verticesadded + 2;

			verticesadded += 3;
			indicesadded += 3;
			vdata += 3;
			idata += 3;
		}
	}

	caster.caster->UnlockIndexBuffer();
	caster.caster->UnlockVertexBuffer();

	shadowindices->Unlock();
	shadowvolume->Unlock();

	// draw
	extrude->SetTechnique("extrude");
	extrude->SetMatrix("matWorld", &caster.world);
	extrude->SetMatrix("matViewProj", &viewproj);

	extrude->Begin(0, 0);
	extrude->BeginPass(0);
	{
		device->SetVertexDeclaration(shadowdecl);
		device->SetStreamSource(0, shadowvolume, 0, sizeof(D3DXVECTOR4));
		device->SetIndices(shadowindices);

		device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, verticesadded, 0, indicesadded / 3);
	}
	extrude->EndPass();
	extrude->End();
}
//*************************************************************************************************************
