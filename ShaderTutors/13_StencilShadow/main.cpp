//*************************************************************************************************************
#include <d3dx9.h>
#include <iostream>
#include <string>
#include <set>
#include <vector>

#include "../common/dxext.h"

// helper macros
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define MYVALID(x)			{ if( FAILED(hr = x) ) { MYERROR(#x); return hr; } }
#define SAFE_RELEASE(x)		{ if( (x) ) { (x)->Release(); (x) = NULL; } }

// external variables
extern long screenwidth;
extern long screenheight;

extern LPDIRECT3DDEVICE9 device;

struct Edge
{
	DWORD i1, i2;				// vertex indices
	D3DXVECTOR3 v1, v2;			// vertex positions

	mutable DWORD other;		// other triangle
	mutable D3DXVECTOR3 n1, n2;	// triangle normals
	mutable bool flip;			// flip volume quad

	Edge() {
		i1 = i2 = 0;
		other = 0xffffffff;
		flip = false;
	}

	inline bool operator <(const Edge& other) const {
		return ((i1 < other.i1) || (i1 == other.i1 && i2 < other.i2));
	}
};

// tutorial variables
typedef std::set<Edge> edgeset;
typedef std::vector<Edge> edgelist;
typedef edgeset::iterator edgeit;

LPD3DXEFFECT		specular		= NULL;
LPD3DXMESH			shadowreceiver	= NULL;
LPD3DXMESH			shadowcaster	= NULL;
LPDIRECT3DTEXTURE9	texture1		= NULL;
LPDIRECT3DTEXTURE9	texture2		= NULL;
edgeset				casteredges;
edgelist			silhouette;

void GenerateEdges(edgeset& out, LPD3DXMESH mesh);
void FindSilhouette(edgelist& out, const edgeset& edges, const D3DXMATRIX& world, const D3DXVECTOR3& lightpos);
void DrawShadowVolume(const edgelist& silhouette, const D3DXMATRIX& world, const D3DXVECTOR3& lightpos);

HRESULT InitScene()
{
	HRESULT hr;
	
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	MYVALID(D3DXLoadMeshFromX("../media/meshes/box.X", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &shadowreceiver));
	MYVALID(D3DXCreateSphere(device, 0.5f, 30, 30, &shadowcaster, NULL));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/marble.dds", &texture1));
	MYVALID(D3DXCreateTextureFromFileA(device, "../media/textures/wood2.jpg", &texture2));

	MYVALID(DXCreateEffect("../media/shaders/blinnphong.fx", device, &specular));

	std::cout << "Generating edge info...\n";
	GenerateEdges(casteredges, shadowcaster);

	return S_OK;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_RELEASE(shadowreceiver);
	SAFE_RELEASE(shadowcaster);
	SAFE_RELEASE(specular);
	SAFE_RELEASE(texture1);
	SAFE_RELEASE(texture2);

	casteredges.clear();
	silhouette.clear();
}
//*************************************************************************************************************
void Update(float delta)
{
	// do nothing
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	D3DXMATRIX		view, proj, vp;
	D3DXMATRIX		world[2];
	D3DXMATRIX		inv;
	D3DXVECTOR3		lightpos(-1, 6, 5);
	D3DXVECTOR3		eye(-3, 3, -3);
	D3DXVECTOR3		look(0, 0.5f, 0);
	D3DXVECTOR3		up(0, 1, 0);
	D3DXVECTOR3		p1, p2;

	// setup camera
	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4, (float)screenwidth / (float)screenheight, 0.1f, 100);

	// put farplane to infinity
	proj._33 = 1;
	proj._43 = -0.1f;

	D3DXMatrixLookAtLH(&view, &eye, &look, &up);
	D3DXMatrixMultiply(&vp, &view, &proj);

	// setup world matrices
	D3DXMatrixScaling(&world[1], 5, 0.1f, 5);
	D3DXMatrixScaling(&world[0], 1.5f, 1.5f, 1.5f);

	world[0]._41 = -0.7f;
	world[0]._42 = 1.3f;
	world[0]._43 = 0.7f;

	time += elapsedtime;

	// specular effect uniforms
	specular->SetMatrix("matViewProj", &vp);
	specular->SetVector("eyePos", (D3DXVECTOR4*)&eye);
	specular->SetVector("lightPos", (D3DXVECTOR4*)&lightpos);

	if( SUCCEEDED(device->BeginScene()) )
	{
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff6694ed, 1.0f, 0);

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
			shadowcaster->DrawSubset(0);
		}
		specular->EndPass();
		specular->End();

		device->SetTexture(0, 0);

		// draw shadow with depth fail method
		D3DXMatrixIdentity(&inv);

		device->SetTransform(D3DTS_WORLD, &inv);
		device->SetTransform(D3DTS_VIEW, &view);
		device->SetTransform(D3DTS_PROJECTION, &proj);

		device->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCRSAT);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

		FindSilhouette(silhouette, casteredges, world[0], (D3DXVECTOR3&)lightpos);
		DrawShadowVolume(silhouette, world[0], lightpos);

		device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_DECRSAT);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

		FindSilhouette(silhouette, casteredges, world[0], (D3DXVECTOR3&)lightpos);
		DrawShadowVolume(silhouette, world[0], lightpos);

		device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA);
		
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
	DWORD		normoff = 0;
	DWORD		stride = mesh->GetNumBytesPerVertex();
	BYTE*		vdata = 0;
	WORD*		idata = 0;
	WORD		i1, i2, i3;
	Edge		e;
	D3DXVECTOR3	p1, p2, p3;
	D3DXVECTOR3	a, b, n;
	edgeit		it;

	out.clear();

	if( is32bit )
	{
		MYERROR("GenerateEdges(): TODO: 4 byte indices");
		return;
	}

	// find the offset of vertex normals
	D3DVERTEXELEMENT9* decl = new D3DVERTEXELEMENT9[MAX_FVF_DECL_SIZE];
	mesh->GetDeclaration(decl);

	for( DWORD i = 0; i < MAX_FVF_DECL_SIZE; ++i )
	{
		if( decl[i].Usage == D3DDECLUSAGE_NORMAL )
		{
			normoff = decl[i].Offset;
			break;
		}

		if( decl[i].Stream == 0xff )
			break;
	}

	delete[] decl;

	if( normoff == 0 )
	{
		MYERROR("GenerateEdges(): Mesh has no normals");
		return;
	}

	// generate edge info
	mesh->LockIndexBuffer(D3DLOCK_READONLY, (LPVOID*)&idata);
	mesh->LockVertexBuffer(D3DLOCK_READONLY, (LPVOID*)&vdata);

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

		if( i1 < i2 )
		{
			e.i1 = i1;
			e.i2 = i2;
			e.v1 = p1;
			e.v2 = p2;
			e.n1 = n;

			if( !out.insert(e).second )
				std::cout << "Crack in object (first triangle)\n";
		}

		if( i2 < i3 )
		{
			e.i1 = i2;
			e.i2 = i3;
			e.v1 = p2;
			e.v2 = p3;
			e.n1 = n;

			if( !out.insert(e).second )
				std::cout << "Crack in object (first triangle)\n";
		}

		if( i3 < i1 )
		{
			e.i1 = i3;
			e.i2 = i1;
			e.v1 = p3;
			e.v2 = p1;
			e.n1 = n;

			if( !out.insert(e).second )
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

			it = out.find(e);

			if( it == out.end() )
			{
				std::cout << "Lone triangle\n";
				continue;
			}

			if( it->other == 0xffffffff )
			{
				it->other = i / 3;
				it->n2 = n;
			}
			else
				std::cout << "Crack in object (second triangle)\n";
		}

		if( i2 > i3 )
		{
			e.i1 = i3;
			e.i2 = i2;

			it = out.find(e);

			if( it->other == 0xffffffff )
			{
				it->other = i / 3;
				it->n2 = n;
			}
			else
				std::cout << "Crack in object (second triangle)\n";
		}

		if( i3 > i1 )
		{
			e.i1 = i1;
			e.i2 = i3;

			it = out.find(e);

			if( it->other == 0xffffffff )
			{
				it->other = i / 3;
				it->n2 = n;
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
	float a, b;

	out.clear();
	out.reserve(50);

	for( edgeset::iterator it = edges.begin(); it != edges.end(); ++it )
	{
		if( it->other != 0xffffffff )
		{
			D3DXVec3TransformCoord(&p1, &it->v1, &world);
			D3DXVec3TransformNormal(&n1, &it->n1, &world);
			D3DXVec3TransformNormal(&n2, &it->n2, &world);

			a = D3DXVec3Dot(&lightpos, &n1) - D3DXVec3Dot(&n1, &p1);
			b = D3DXVec3Dot(&lightpos, &n2) - D3DXVec3Dot(&n2, &p1);

			if( (a < 0) != (b < 0) )
			{
				if( out.capacity() <= out.size() )
					out.reserve(out.size() + 50);

				it->flip = (b < 0);
				out.push_back(*it);
			}
		}
	}
}
//*************************************************************************************************************
void DrawShadowVolume(const edgelist& silhouette, const D3DXMATRIX& world, const D3DXVECTOR3& lightpos)
{
	D3DXVECTOR3 p1, p2;
	size_t floatsperquad = 6 * 4;
	float* vdata = new float[silhouette.size() * floatsperquad];

	for( size_t i = 0; i < silhouette.size() * floatsperquad; i += floatsperquad )
	{
		const Edge& e = silhouette[i / floatsperquad];

		D3DXVec3TransformCoord(&p1, &e.v1, &world);
		D3DXVec3TransformCoord(&p2, &e.v2, &world);

		if( e.flip )
		{
			vdata[i + 0] = p1.x;	vdata[i + 4] = p1.x - lightpos.x;	vdata[i + 8] = p2.x;
			vdata[i + 1] = p1.y;	vdata[i + 5] = p1.y - lightpos.y;	vdata[i + 9] = p2.y;
			vdata[i + 2] = p1.z;	vdata[i + 6] = p1.z - lightpos.z;	vdata[i + 10] = p2.z;
			vdata[i + 3] = 1;		vdata[i + 7] = 0;					vdata[i + 11] = 1;

			vdata[i + 12] = p2.x;	vdata[i + 16] = p1.x - lightpos.x;	vdata[i + 20] = p2.x - lightpos.x;
			vdata[i + 13] = p2.y;	vdata[i + 17] = p1.y - lightpos.y;	vdata[i + 21] = p2.y - lightpos.y;
			vdata[i + 14] = p2.z;	vdata[i + 18] = p1.z - lightpos.z;	vdata[i + 22] = p2.z - lightpos.z;
			vdata[i + 15] = 1;		vdata[i + 19] = 0;		vdata[i + 23] = 0;
		}
		else
		{
			vdata[i + 0] = p1.x;	vdata[i + 8] = p1.x - lightpos.x;	vdata[i + 4] = p2.x;
			vdata[i + 1] = p1.y;	vdata[i + 9] = p1.y - lightpos.y;	vdata[i + 5] = p2.y;
			vdata[i + 2] = p1.z;	vdata[i + 10] = p1.z - lightpos.z;	vdata[i + 6] = p2.z;
			vdata[i + 3] = 1;		vdata[i + 11] = 0;					vdata[i + 7] = 1;

			vdata[i + 12] = p2.x;	vdata[i + 20] = p1.x - lightpos.x;	vdata[i + 16] = p2.x - lightpos.x;
			vdata[i + 13] = p2.y;	vdata[i + 21] = p1.y - lightpos.y;	vdata[i + 17] = p2.y - lightpos.y;
			vdata[i + 14] = p2.z;	vdata[i + 22] = p1.z - lightpos.z;	vdata[i + 18] = p2.z - lightpos.z;
			vdata[i + 15] = 1;		vdata[i + 23] = 0;					vdata[i + 19] = 0;
		}
	}

	device->SetFVF(D3DFVF_XYZW);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, silhouette.size() * 2, vdata, sizeof(D3DXVECTOR4));

	delete[] vdata;
}
//*************************************************************************************************************
