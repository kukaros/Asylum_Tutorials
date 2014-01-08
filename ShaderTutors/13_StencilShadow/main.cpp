//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>
#include <set>
#include <vector>

#include "../common/dxext.h"
#include "../common/orderedarray.hpp"

// TODO:
// - sphere texcoord is not correct
// - two-sided stencil
// - needs tonemap

#define NUM_OBJECTS			3
#define VOLUME_OFFSET		2e-2f	// to avoid zfight

// not reliable
#define VOLUME_NUMVERTICES(x)	max(128, x * 2)
#define VOLUME_NUMINDICES(x)	max(256, x * 3)		// x is numfaces

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
	LPD3DXMESH	object;			// for normal rendering
	LPD3DXMESH	caster;			// for silhouette determination
	
	LPDIRECT3DVERTEXBUFFER9	vertices;		// volume vertices
	LPDIRECT3DINDEXBUFFER9	indices;		// volume indices
	DWORD					numvertices;	// volume vertices
	DWORD					numfaces;		// volume faces

	ShadowCaster()
	{
		object = caster = 0;
		vertices = 0;
		indices = 0;
	}
};

float textvertices[36] =
{
	9.5f,			9.5f,	0, 1,	0, 0,
	521.5f,			9.5f,	0, 1,	1, 0,
	9.5f,	128.0f + 9.5f,	0, 1,	0, 1,

	9.5f,	128.0f + 9.5f,	0, 1,	0, 1,
	521.5f,			9.5f,	0, 1,	1, 0,
	521.5f,	128.0f + 9.5f,	0, 1,	1, 1
};

// tutorial variables
LPD3DXEFFECT					ambient			= NULL;
LPD3DXEFFECT					specular		= NULL;
LPD3DXEFFECT					extrude			= NULL;
LPD3DXMESH						shadowreceiver	= NULL;
LPDIRECT3DTEXTURE9				texture1		= NULL;
LPDIRECT3DTEXTURE9				texture2		= NULL;
LPDIRECT3DTEXTURE9				text			= NULL;
LPDIRECT3DVERTEXDECLARATION9	shadowdecl		= NULL;

state<D3DXVECTOR2>				cameraangle;
state<D3DXVECTOR2>				lightangle;
ShadowCaster					objects[NUM_OBJECTS]; // leak
bool							drawsilhouette	= false;
bool							drawvolume		= false;

// tutorial functions
void GenerateEdges(edgeset& out, LPD3DXMESH mesh);
void FindSilhouette(ShadowCaster& caster, const D3DXVECTOR3& lightpos);
void ExtrudeSilhouette(ShadowCaster& caster, const D3DXVECTOR3& lightpos);
void DrawScene(LPD3DXEFFECT effect, bool texture);
void DrawShadowVolume(const ShadowCaster& caster);

HRESULT InitScene()
{
	HRESULT hr;

	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2.jpg", &texture2));
	MYVALID(DXCreateEffect("../media/shaders/ambient.fx", device, &ambient));
	MYVALID(DXCreateEffect("../media/shaders/blinnphong.fx", device, &specular));
	MYVALID(DXCreateEffect("../media/shaders/extrude.fx", device, &extrude));

	MYVALID(device->CreateTexture(512, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &text, NULL));
	MYVALID(DXCreateTexturedBox(device, D3DXMESH_MANAGED, &shadowreceiver));

	// box
	MYVALID(DXCreateTexturedBox(device, D3DXMESH_MANAGED, &objects[0].object));
	MYVALID(DXCreateCollisionBox(device, D3DXMESH_SYSTEMMEM, &objects[0].caster));

	D3DXMatrixTranslation(&objects[0].world, -1.5f, 0.55f, 1.2f);

	MYVALID(device->CreateVertexBuffer(VOLUME_NUMVERTICES(objects[0].caster->GetNumVertices()) * sizeof(D3DXVECTOR4), D3DUSAGE_DYNAMIC, D3DFVF_XYZW, D3DPOOL_DEFAULT, &objects[0].vertices, NULL));
	MYVALID(device->CreateIndexBuffer(VOLUME_NUMINDICES(objects[0].caster->GetNumFaces()) * sizeof(WORD), D3DUSAGE_DYNAMIC, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &objects[0].indices, NULL));

	// sphere
	MYVALID(DXCreateTexturedSphere(device, 0.5f, 30, 30, D3DXMESH_SYSTEMMEM, &objects[1].object));
	MYVALID(DXCreateCollisionSphere(device, 0.5f, 30, 30, D3DXMESH_SYSTEMMEM, &objects[1].caster));

	D3DXMatrixTranslation(&objects[1].world, 1.0f, 0.55f, 0.7f);

	MYVALID(device->CreateVertexBuffer(VOLUME_NUMVERTICES(objects[1].caster->GetNumVertices()) * sizeof(D3DXVECTOR4), D3DUSAGE_DYNAMIC, D3DFVF_XYZW, D3DPOOL_DEFAULT, &objects[1].vertices, NULL));
	MYVALID(device->CreateIndexBuffer(VOLUME_NUMINDICES(objects[1].caster->GetNumFaces()) * sizeof(WORD), D3DUSAGE_DYNAMIC, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &objects[1].indices, NULL));

	// L shape
	MYVALID(DXCreateTexturedLShape(device, D3DXMESH_MANAGED, &objects[2].object));
	MYVALID(DXCreateCollisionLShape(device, D3DXMESH_SYSTEMMEM, &objects[2].caster));

	D3DXMatrixTranslation(&objects[2].world, 0, 0.55f, -1);

	MYVALID(device->CreateVertexBuffer(VOLUME_NUMVERTICES(objects[2].caster->GetNumVertices()) * sizeof(D3DXVECTOR4), D3DUSAGE_DYNAMIC, D3DFVF_XYZW, D3DPOOL_DEFAULT, &objects[2].vertices, NULL));
	MYVALID(device->CreateIndexBuffer(VOLUME_NUMINDICES(objects[2].caster->GetNumFaces()) * sizeof(WORD), D3DUSAGE_DYNAMIC, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &objects[2].indices, NULL));

#if 0
	D3DXMATERIAL defmat;

	defmat.pTextureFilename = 0;
	defmat.MatD3D.Ambient = D3DXCOLOR(1, 1, 1, 1);
	defmat.MatD3D.Diffuse = D3DXCOLOR(1, 1, 1, 1);
	defmat.MatD3D.Specular = D3DXCOLOR(1, 1, 1, 1);
	defmat.MatD3D.Emissive = D3DXCOLOR(0, 0, 0, 0);
	defmat.MatD3D.Power = 20;

	DXSaveMeshToQM("../media/meshes10/box.qm", objects[0].object, &defmat, 1);
	DXSaveMeshToQM("../media/meshes10/collisionbox.qm", objects[0].caster, &defmat, 1);

	DXSaveMeshToQM("../media/meshes10/sphere.qm", objects[1].object, &defmat, 1);
	DXSaveMeshToQM("../media/meshes10/collisionsphere.qm", objects[1].caster, &defmat, 1);

	DXSaveMeshToQM("../media/meshes10/lshape.qm", objects[2].object, &defmat, 1);
	DXSaveMeshToQM("../media/meshes10/collisionlshape.qm", objects[2].caster, &defmat, 1);

#endif

	// generate edges
	std::cout << "Generating edge info...\n";

	for( int i = 0; i < NUM_OBJECTS; ++i )
		GenerateEdges(objects[i].edges, objects[i].caster);

	// shadow volume decl
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		D3DDECL_END()
	};

	MYVALID(device->CreateVertexDeclaration(elem, &shadowdecl));

	// effect
	specular->SetFloat("ambient", 0);

	cameraangle = D3DXVECTOR2(0.78f, 0.78f);
	lightangle = D3DXVECTOR2(2.8f, 0.78f);

	DXRenderText("Use mouse to rotate camera and light\n\n1: draw silhouette\n2: draw shadow volume", text, 512, 128);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	for( int i = 0; i < NUM_OBJECTS; ++i )
	{
		SAFE_RELEASE(objects[i].object);
		SAFE_RELEASE(objects[i].caster);

		SAFE_RELEASE(objects[i].vertices);
		SAFE_RELEASE(objects[i].indices);

		objects[i].edges.clear();
		objects[i].edges.swap(edgeset());

		objects[i].silhouette.clear();
		objects[i].silhouette.swap(edgelist());
	}

	SAFE_RELEASE(text);
	SAFE_RELEASE(shadowdecl);
	SAFE_RELEASE(shadowreceiver);
	SAFE_RELEASE(specular);
	SAFE_RELEASE(ambient);
	SAFE_RELEASE(extrude);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(texture2);
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	if( wparam == 0x31 )
		drawsilhouette = !drawsilhouette;
	else if( wparam == 0x32 )
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
void DrawScene(LPD3DXEFFECT effect)
{
	D3DXMATRIX	world;
	D3DXMATRIX	inv;
	D3DXVECTOR4	uv(3, 3, 0, 0);

	D3DXMatrixScaling(&world, 5, 0.1f, 5);
	D3DXMatrixInverse(&inv, NULL, &world);

	effect->SetMatrix("matWorld", &world);
	effect->SetMatrix("matWorldInv", &inv);
	effect->SetVector("uv", &uv);

	effect->Begin(0, 0);
	effect->BeginPass(0);
	{
		device->SetTexture(0, texture2);
		shadowreceiver->DrawSubset(0);

		if( !drawsilhouette )
		{
			device->SetTexture(0, texture1);

			uv.x = uv.y = 1;
			effect->SetVector("uv", &uv);

			for( int i = 0; i < NUM_OBJECTS; ++i )
			{
				D3DXMatrixInverse(&inv, NULL, &objects[i].world);

				effect->SetMatrix("matWorld", &objects[i].world);
				effect->SetMatrix("matWorldInv", &inv);
				effect->CommitChanges();

				objects[i].object->DrawSubset(0);
			}
		}
	}
	effect->EndPass();
	effect->End();
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	D3DXMATRIX		view, proj, vp;
	D3DXMATRIX		world;
	D3DXMATRIX		inv;

	D3DXVECTOR4		amblight(0.2f, 0.2f, 0.2f, 1);
	D3DXVECTOR4		intensity(0.8f, 0.8f, 0.8f, 1);
	D3DXVECTOR4		zero(0, 0, 0, 1);

	D3DXVECTOR3		lightpos(0, 0, -10);
	D3DXVECTOR3		eye(0, 0, -5.2f);
	D3DXVECTOR3		look(0, 0.5f, 0);
	D3DXVECTOR3		up(0, 1, 0);
	D3DXVECTOR3		p1, p2;
	D3DXVECTOR2		orient	= cameraangle.smooth(alpha);
	D3DXVECTOR2		light	= lightangle.smooth(alpha);

	time += elapsedtime;

	// setup light
	D3DXMatrixRotationYawPitchRoll(&view, light.x, light.y, 0);
	D3DXVec3TransformCoord(&lightpos, &lightpos, &view);

	// TODO: no need to calculate every frame
	for( int i = 0; i < NUM_OBJECTS; ++i )
	{
		FindSilhouette(objects[i], (D3DXVECTOR3&)lightpos);
		ExtrudeSilhouette(objects[i], (D3DXVECTOR3&)lightpos);
	}

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

	// specular effect uniforms
	specular->SetMatrix("matViewProj", &vp);
	specular->SetVector("eyePos", (D3DXVECTOR4*)&eye);
	specular->SetVector("lightPos", (D3DXVECTOR4*)&lightpos);
	specular->SetVector("ambient", &zero); // it's a fuck-up
	specular->SetVector("intensity", &intensity); // lazy to tonemap
	
	ambient->SetMatrix("matViewProj", &vp);
	ambient->SetVector("ambient", &amblight);

	if( SUCCEEDED(device->BeginScene()) )
	{
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0xff6694ed, 1.0f, 0);

		// STEP 1: z pass
		ambient->SetTechnique("ambientlight");
		ambient->SetMatrix("matViewProj", &vp);

		DrawScene(ambient);

		// STEP 2: draw shadow with depth fail method
		device->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCR);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

		extrude->SetTechnique("extrude");
		extrude->SetMatrix("matViewProj", &vp);

		extrude->Begin(0, 0);
		extrude->BeginPass(0);
		{
			for( int i = 0; i < NUM_OBJECTS; ++i )
				DrawShadowVolume(objects[i]);

			device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_DECR);
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

			for( int i = 0; i < NUM_OBJECTS; ++i )
				DrawShadowVolume(objects[i]);
		}
		extrude->EndPass();
		extrude->End();

		device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA);

		// STEP 3: multipass lighting
		device->SetRenderState(D3DRS_ZENABLE, TRUE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

		device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_GREATER);
		device->SetRenderState(D3DRS_STENCILREF, 1);

		DrawScene(specular);

		device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		if( drawsilhouette )
		{
			amblight = D3DXVECTOR4(1, 1, 0, 0.5f);

			// reuse whatever we can...
			extrude->SetVector("ambient", &amblight);
			extrude->Begin(0, 0);
			extrude->BeginPass(0);

			device->SetVertexDeclaration(shadowdecl);

			for( int i = 0; i < NUM_OBJECTS; ++i )
			{
				const ShadowCaster& caster = objects[i];
				D3DXVECTOR4* verts = (D3DXVECTOR4*)malloc(caster.silhouette.size() * 2 * sizeof(D3DXVECTOR4));

				for( size_t j = 0; j < caster.silhouette.size(); ++j )
				{
					const Edge& e = caster.silhouette[j];

					verts[j * 2 + 0] = D3DXVECTOR4(e.v1, 1);
					verts[j * 2 + 1] = D3DXVECTOR4(e.v2, 1);
				}

				extrude->SetMatrix("matWorld", &caster.world);
				extrude->CommitChanges();

				device->DrawPrimitiveUP(D3DPT_LINELIST, caster.silhouette.size(), verts, sizeof(D3DXVECTOR4));
				free(verts);
			}

			extrude->EndPass();
			extrude->End();
			extrude->SetVector("ambient", &zero);
		}

		if( drawvolume )
		{
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			amblight = D3DXVECTOR4(1, 1, 0, 0.5f);

			extrude->SetVector("ambient", &amblight);
			extrude->Begin(0, 0);
			extrude->BeginPass(0);

			for( int i = 0; i < NUM_OBJECTS; ++i )
				DrawShadowVolume(objects[i]);

			extrude->EndPass();
			extrude->End();
			extrude->SetVector("ambient", &zero);

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		}

		// render text
		device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		device->SetTexture(0, text);
		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, textvertices, 6 * sizeof(float));

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		device->SetRenderState(D3DRS_ZENABLE, TRUE);

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
void ExtrudeSilhouette(ShadowCaster& caster, const D3DXVECTOR3& lightpos)
{
	D3DXVECTOR4*	vdata = 0;
	WORD*			idata = 0;
	D3DXMATRIX		inv;
	D3DXVECTOR3		lp;
	D3DXVECTOR3		center(0, 0, 0);
	D3DXVECTOR3		ltocenter;
	float			weight = 0.5f / caster.silhouette.size();

	caster.vertices->Lock(0, 0, (void**)&vdata, D3DLOCK_DISCARD);
	caster.indices->Lock(0, 0, (void**)&idata, D3DLOCK_DISCARD);

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
	WORD*			added = (WORD*)malloc(caster.caster->GetNumVertices() * sizeof(WORD));

	memset(added, 0, caster.caster->GetNumVertices() * sizeof(WORD));

	//vdata += caster.silhouette.size() * 4 + 1;
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

		// OPTIM: store normals
		a = v2->operator -(*v1);
		b = v3->operator -(*v1);

		D3DXVec3Cross(&n, &a, &b);
		dist = D3DXVec3Dot(&n, &lp) - D3DXVec3Dot(&n, v1);

		if( dist > 0 )
		{
			if( !added[i1] )
			{
				vdata[verticesadded] = D3DXVECTOR4(*v1 + ltocenter, 1);
				added[i1] = verticesadded;
				++verticesadded;
			}

			if( !added[i2] )
			{
				vdata[verticesadded] = D3DXVECTOR4(*v2 + ltocenter, 1);
				added[i2] = verticesadded;
				++verticesadded;
			}

			if( !added[i3] )
			{
				vdata[verticesadded] = D3DXVECTOR4(*v3 + ltocenter, 1);
				added[i3] = verticesadded;
				++verticesadded;
			}

			idata[0] = added[i1];
			idata[1] = added[i2];
			idata[2] = added[i3];

			indicesadded += 3;
			idata += 3;
		}
	}

	free(added);

	caster.caster->UnlockIndexBuffer();
	caster.caster->UnlockVertexBuffer();

	caster.indices->Unlock();
	caster.vertices->Unlock();

	caster.numvertices = verticesadded;
	caster.numfaces = indicesadded / 3;

#ifdef _DEBUG
	// as kissing frogs...
	DWORD test1 = VOLUME_NUMVERTICES(caster.caster->GetNumVertices());
	DWORD test2 = VOLUME_NUMINDICES(caster.caster->GetNumFaces());

	if( (verticesadded > test1) ||
		(indicesadded > test2) )
	{
		std::cout << "Buffer overflow!\n";
		::_CrtDbgBreak();
	}
#endif
}
//*************************************************************************************************************
void DrawShadowVolume(const ShadowCaster& caster)
{
	extrude->SetMatrix("matWorld", &caster.world);
	extrude->CommitChanges();
	
	device->SetVertexDeclaration(shadowdecl);
	device->SetStreamSource(0, caster.vertices, 0, sizeof(D3DXVECTOR4));
	device->SetIndices(caster.indices);
	device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, caster.numvertices, 0, caster.numfaces);
}
//*************************************************************************************************************
