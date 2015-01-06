
#include "dxext.h"

#include <GdiPlus.h>

#include <iostream>
#include <cstdio>

ULONG_PTR						gdiplustoken = 0;
LPDIRECT3DVERTEXDECLARATION9	blurdeclforpoint = 0;
LPDIRECT3DVERTEXDECLARATION9	blurdeclforpointfordirectional = 0;

static const D3DXVECTOR3 DXCubeForward[6] =
{
	D3DXVECTOR3(1, 0, 0),
	D3DXVECTOR3(-1, 0, 0),
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, -1, 0),
	D3DXVECTOR3(0, 0, 1),
	D3DXVECTOR3(0, 0, -1),
};

static const D3DXVECTOR3 DXCubeUp[6] =
{
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, 0, -1),
	D3DXVECTOR3(0, 0, 1),
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, 1, 0),
};

static const float DXCubeVertices[24 * 5] =
{
	// X+
	1, -1, 1,	0, 1,
	1, 1, 1,	0, 0,
	1, 1, -1,	1, 0,
	1, -1, -1,	1, 1,

	// X-
	-1, -1, -1,	0, 1,
	-1, 1, -1,	0, 0,
	-1, 1, 1,	1, 0,
	-1, -1, 1,	1, 1,

	// Y+
	-1, 1, 1,	0, 1,
	-1, 1, -1,	0, 0,
	1, 1, -1,	1, 0,
	1, 1, 1,	1, 1,

	// Y-
	-1, -1, -1,	0, 1,
	-1, -1, 1,	0, 0,
	1, -1, 1,	1, 0,
	1, -1, -1,	1, 1,

	// Z+
	-1, -1, 1,	0, 1,
	-1, 1, 1,	0, 0,
	1, 1, 1,	1, 0,
	1, -1, 1,	1, 1,

	// Z-
	1, -1, -1,	0, 1,
	1, 1, -1,	0, 0,
	-1, 1, -1,	1, 0,
	-1, -1, -1,	1, 1,
};

static const unsigned short DXCubeIndices[6] =
{
	0, 1, 2, 0, 2, 3
};

static void DXReadString(FILE* f, char* buff)
{
	size_t ind = 0;
	char ch = fgetc(f);

	while( ch != '\n' )
	{
		buff[ind] = ch;
		ch = fgetc(f);
		++ind;
	}

	buff[ind] = '\0';
}

// *****************************************************************************************************************************
//
// OpenGLAABox impl
//
// *****************************************************************************************************************************

#define ASGN_IF(a, op, b) \
	if( (a) op (b) ) (a) = (b);

DXAABox::DXAABox()
{
	Min[0] = Min[1] = Min[2] = FLT_MAX;
	Max[0] = Max[1] = Max[2] = -FLT_MAX;
}

void DXAABox::Add(float x, float y, float z)
{
	ASGN_IF(Max[0], <, x);
	ASGN_IF(Max[1], <, y);
	ASGN_IF(Max[2], <, z);

	ASGN_IF(Min[0], >, x);
	ASGN_IF(Min[1], >, y);
	ASGN_IF(Min[2], >, z);
}

void DXAABox::Add(const D3DXVECTOR3& v)
{
	ASGN_IF(Max[0], <, v[0]);
	ASGN_IF(Max[1], <, v[1]);
	ASGN_IF(Max[2], <, v[2]);

	ASGN_IF(Min[0], >, v[0]);
	ASGN_IF(Min[1], >, v[1]);
	ASGN_IF(Min[2], >, v[2]);
}

void DXAABox::GetCenter(D3DXVECTOR3& out) const
{
	out[0] = (Min[0] + Max[0]) * 0.5f;
	out[1] = (Min[1] + Max[1]) * 0.5f;
	out[2] = (Min[2] + Max[2]) * 0.5f;
}

void DXAABox::GetHalfSize(D3DXVECTOR3& out) const
{
	out[0] = (Max[0] - Min[0]) * 0.5f;
	out[1] = (Max[1] - Min[1]) * 0.5f;
	out[2] = (Max[2] - Min[2]) * 0.5f;
}

void DXAABox::TransformAxisAligned(const D3DXMATRIX& traf)
{
	D3DXVECTOR3 vertices[8] =
	{
		D3DXVECTOR3(Min[0], Min[1], Min[2]),
		D3DXVECTOR3(Min[0], Min[1], Max[2]),
		D3DXVECTOR3(Min[0], Max[1], Min[2]),
		D3DXVECTOR3(Min[0], Max[1], Max[2]),
		D3DXVECTOR3(Max[0], Min[1], Min[2]),
		D3DXVECTOR3(Max[0], Min[1], Max[2]),
		D3DXVECTOR3(Max[0], Max[1], Min[2]),
		D3DXVECTOR3(Max[0], Max[1], Max[2])
	};
	
	for( int i = 0; i < 8; ++i )
		D3DXVec3TransformCoord(&vertices[i], &vertices[i], &traf);

	Min[0] = Min[1] = Min[2] = FLT_MAX;
	Max[0] = Max[1] = Max[2] = -FLT_MAX;

	for( int i = 0; i < 8; ++i )
		Add(vertices[i]);
}

void DXAABox::GetPlanes(D3DXPLANE outplanes[6]) const
{
#define CALC_PLANE(i, nx, ny, nz, px, py, pz) \
	outplanes[i].a = nx;	p[0] = px; \
	outplanes[i].b = ny;	p[1] = py; \
	outplanes[i].c = nz;	p[2] = pz; \
	outplanes[i].d = -D3DXVec3Dot(&p, (D3DXVECTOR3*)&outplanes[i]); \
	D3DXPlaneNormalize(&outplanes[i], &outplanes[i]);
// END

	D3DXVECTOR3 p;

	CALC_PLANE(0, 1, 0, 0, Min[0], Min[1], Min[2]);		// left
	CALC_PLANE(1, -1, 0, 0, Max[0], Min[1], Min[2]);	// right
	CALC_PLANE(2, 0, 1, 0, Min[0], Min[1], Min[2]);		// bottom
	CALC_PLANE(3, 0, -1, 0, Min[0], Max[1], Min[2]);	// top
	CALC_PLANE(4, 0, 0, -1, Min[0], Min[1], Max[2]);	// front
	CALC_PLANE(5, 0, 0, 1, Min[0], Min[1], Min[2]);		// back
}

float DXAABox::Radius() const
{
	D3DXVECTOR3 tmp = Min - Max;
	return D3DXVec3Length(&tmp) * 0.5f;
}

float DXAABox::Nearest(const D3DXPLANE& from) const
{
#define FAST_DISTANCE(x, y, z, p, op) \
	d = p[0] * x + p[1] * y + p[2] * z + p[3]; \
	ASGN_IF(dist, op, d);
// END

	float d, dist = FLT_MAX;

	FAST_DISTANCE(Min[0], Min[1], Min[2], from, >);
	FAST_DISTANCE(Min[0], Min[1], Max[2], from, >);
	FAST_DISTANCE(Min[0], Max[1], Min[2], from, >);
	FAST_DISTANCE(Min[0], Max[1], Max[2], from, >);
	FAST_DISTANCE(Max[0], Min[1], Min[2], from, >);
	FAST_DISTANCE(Max[0], Min[1], Max[2], from, >);
	FAST_DISTANCE(Max[0], Max[1], Min[2], from, >);
	FAST_DISTANCE(Max[0], Max[1], Max[2], from, >);

	return dist;
}

float DXAABox::Farthest(const D3DXPLANE& from) const
{
	float d, dist = -FLT_MAX;

	FAST_DISTANCE(Min[0], Min[1], Min[2], from, <);
	FAST_DISTANCE(Min[0], Min[1], Max[2], from, <);
	FAST_DISTANCE(Min[0], Max[1], Min[2], from, <);
	FAST_DISTANCE(Min[0], Max[1], Max[2], from, <);
	FAST_DISTANCE(Max[0], Min[1], Min[2], from, <);
	FAST_DISTANCE(Max[0], Min[1], Max[2], from, <);
	FAST_DISTANCE(Max[0], Max[1], Min[2], from, <);
	FAST_DISTANCE(Max[0], Max[1], Max[2], from, <);

	return dist;
}

// *****************************************************************************************************************************
//
// DXObject impl
//
// *****************************************************************************************************************************

DXObject::DXObject(IDirect3DDevice9* d3ddevice)
{
	device = d3ddevice;
	mesh = NULL;
	textures = NULL;
	materials = NULL;
}

DXObject::~DXObject()
{
	Clean();
}

void DXObject::Clean()
{
	if( mesh )
		mesh->Release();

	if( textures )
	{
		for( DWORD i = 0; i < nummaterials; ++i )
		{
			if( textures[i] )
				textures[i]->Release();
		}

		delete[] textures;
	}

	if( materials )
	{
		for( DWORD i = 0; i < nummaterials; ++i )
		{
			if( materials[i].pTextureFilename )
				delete[] materials[i].pTextureFilename;
		}

		delete[] materials;
	}

	mesh = NULL;
	textures = NULL;
	materials = NULL;
}

bool DXObject::GenerateTangentFrame()
{
	return SUCCEEDED(DXGenTangentFrame(device, &mesh));
}

bool DXObject::Load(const std::string& file)
{
	HRESULT			hr;
	LPD3DXBUFFER	buffer = NULL;
	std::string		path;
	std::string		ext;
	char*			matname;
	int				namelen;

	size_t pos = file.find_last_of("\\/");

	if( pos != std::string::npos )
		path = file.substr(0, pos + 1);

	pos = file.find_last_of(".");

	if( pos != std::string::npos )
		ext = file.substr(pos + 1);

	Clean();

	if( ext == "x" || ext == "X" )
		hr = D3DXLoadMeshFromX(file.c_str(), D3DXMESH_MANAGED, device, NULL, &buffer, NULL, &nummaterials, &mesh);
	else if( ext == "qm" )
		hr = DXLoadMeshFromQM(file.c_str(), D3DXMESH_MANAGED, device, &materials, &nummaterials, &mesh);
	else
		hr = E_FAIL;

	if( FAILED(hr) )
		return false;

	if( nummaterials > 0 )
	{
		if( buffer )
		{
			materials = new D3DXMATERIAL[nummaterials];
			memcpy(materials, buffer->GetBufferPointer(), nummaterials * sizeof(D3DXMATERIAL));
		}

		textures = new IDirect3DTexture9*[nummaterials];
		memset(textures, 0, nummaterials * sizeof(IDirect3DTexture9));

		for( DWORD i = 0; i < nummaterials; ++i )
		{
			if( materials[i].pTextureFilename )
			{
				if( buffer )
				{
					namelen = strlen(materials[i].pTextureFilename);
					matname = new char[namelen + 1];
			
					memcpy(matname, materials[i].pTextureFilename, namelen);
					matname[namelen] = 0;

					materials[i].pTextureFilename = matname;
				}
				else
					matname = materials[i].pTextureFilename;

				hr = D3DXCreateTextureFromFileA(device, (path + matname).c_str(), &textures[i]);

				if( FAILED(hr) )
					return false;
			}
		}
	}

	if( buffer )
		buffer->Release();

	return true;
}

bool DXObject::Save(const std::string& file)
{
	HRESULT hr = DXSaveMeshToQM(file.c_str(), mesh, materials, nummaterials);
	return SUCCEEDED(hr);
}

void DXObject::Draw(unsigned int flags, LPD3DXEFFECT effect)
{
	for( DWORD i = 0; i < nummaterials; ++i )
		DrawSubset(i, flags, effect);
}

void DXObject::DrawExcept(DWORD subset, unsigned int flags, LPD3DXEFFECT effect)
{
	for( DWORD i = 0; i < nummaterials; ++i )
	{
		if( i != subset )
			DrawSubset(i, flags, effect);
	}
}

void DXObject::DrawSubset(DWORD subset, unsigned int flags, LPD3DXEFFECT effect)
{
	D3DXVECTOR4 params(1, 1, 1, 1);

	if( subset < nummaterials )
	{
		if( !(flags & Opaque) && (textures[subset] || materials[subset].MatD3D.Diffuse.a == 1) )
			return;

		if( !(flags & Transparent) && (materials[subset].MatD3D.Diffuse.a < 1) )
			return;

		if( flags & Material )
		{
			if( effect )
			{
				if( !textures[subset] )
					effect->SetVector("matDiffuse", (D3DXVECTOR4*)&materials[subset].MatD3D.Diffuse);

				params.w = (textures[subset] ? 1.0f : 0.0f);

				effect->SetVector("params", &params);
				effect->CommitChanges();
			}

			device->SetTexture(0, textures[subset]);
		}

		mesh->DrawSubset(subset);
	}
}

// *****************************************************************************************************************************
//
// DXLight impl
//
// *****************************************************************************************************************************

DXLight::~DXLight()
{
}

void DXLight::CreateShadowMap(LPDIRECT3DDEVICE9 device, DWORD type, DWORD size)
{
	shadowtype = type;
	shadowsize = size;
}

// *****************************************************************************************************************************
//
// DXDirectionalLight impl
//
// *****************************************************************************************************************************

DXDirectionalLight::DXDirectionalLight(const D3DXCOLOR& color, const D3DXVECTOR4& direction)
{
	this->color = color;
	this->direction = direction;

	shadowmap = 0;
	blur = 0;
	needsredraw = true;
	needsblur = true;
}

DXDirectionalLight::~DXDirectionalLight()
{
	if( shadowmap )
	{
		shadowmap->Release();
		shadowmap = 0;
	}

	if( blur )
	{
		blur->Release();
		blur = 0;
	}

	if( blurdeclforpointfordirectional )
	{
		if( 0 == blurdeclforpointfordirectional->Release() )
			blurdeclforpointfordirectional = 0;
	}
}

void DXDirectionalLight::CreateShadowMap(LPDIRECT3DDEVICE9 device, DWORD type, DWORD size)
{
	HRESULT hr;

	DXLight::CreateShadowMap(device, type, size);
	hr = device->CreateTexture(size, size, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &shadowmap, NULL);

	if( SUCCEEDED(hr) )
		hr = device->CreateTexture(size, size, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &blur, NULL);

	if( FAILED(hr) )
	{
		if( shadowmap )
			shadowmap->Release();

		shadowmap = blur = NULL;
	}

	if( !blurdeclforpointfordirectional )
	{
		D3DVERTEXELEMENT9 elem[] =
		{
			{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
			{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			D3DDECL_END()
		};

		device->CreateVertexDeclaration(elem, &blurdeclforpointfordirectional);
	}
	else
		blurdeclforpointfordirectional->AddRef();
}

void DXDirectionalLight::GetViewProjMatrix(D3DXMATRIX& out, const D3DXVECTOR3& origin)
{
	D3DXVECTOR3 look;
	D3DXVECTOR3 up(0, 1, 0);
	D3DXMATRIX proj;

	look.x = origin.x + direction.w * direction.x;
	look.y = origin.y + direction.w * direction.y;
	look.z = origin.z + direction.w * direction.z;

	D3DXMatrixLookAtLH(&out, &look, &origin, &up);
	D3DXMatrixOrthoLH(&proj, 7, 7, 0.1f, 100);
	D3DXMatrixMultiply(&out, &out, &proj);
}

void DXDirectionalLight::BlurShadowMap(LPDIRECT3DDEVICE9 device, LPD3DXEFFECT effect)
{
	if( !shadowmap || !blur || !blurdeclforpointfordirectional || !needsblur )
		return;

	float blurvertices[36] =
	{
		-0.5f,						-0.5f,						0, 1,	0, 0,
		(float)shadowsize - 0.5f,	-0.5f,						0, 1,	1, 0,
		-0.5f,						(float)shadowsize - 0.5f,	0, 1,	0, 1,
	
		-0.5f,						(float)shadowsize - 0.5f,	0, 1,	0, 1,
		(float)shadowsize - 0.5f,	-0.5f,						0, 1,	1, 0,
		(float)shadowsize - 0.5f,	(float)shadowsize - 0.5f,	0, 1,	1, 1
	};

	LPDIRECT3DSURFACE9	surface = NULL;
	D3DXVECTOR4			texelsize(1.0f / shadowsize, 0, 0, 0);
	UINT				stride = 6 * sizeof(float);

	device->SetVertexDeclaration(blurdeclforpointfordirectional);

	// x
	blur->GetSurfaceLevel(0, &surface);

	effect->SetVector("texelSize", &texelsize);
	effect->CommitChanges();

	device->SetRenderTarget(0, surface);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

	device->SetTexture(0, shadowmap);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, blurvertices, stride);

	surface->Release();
	std::swap(texelsize.x, texelsize.y);

	// y
	shadowmap->GetSurfaceLevel(0, &surface);

	effect->SetVector("texelSize", &texelsize);
	effect->CommitChanges();

	device->SetRenderTarget(0, surface);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

	device->SetTexture(0, blur);
	device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, blurvertices, stride);

	surface->Release();

	if( shadowtype == Static )
		needsblur = false;
}

void DXDirectionalLight::DrawShadowMap(LPDIRECT3DDEVICE9 device, LPD3DXEFFECT effect, void (*drawcallback)(LPD3DXEFFECT))
{
	if( !shadowmap || !needsredraw )
		return;

	LPDIRECT3DSURFACE9 surface = NULL;
	D3DXMATRIX vp;

	GetViewProjMatrix(vp, D3DXVECTOR3(0, 0, 0));

	effect->SetVector("lightPos", &direction);
	effect->SetMatrix("matViewProj", &vp);

	shadowmap->GetSurfaceLevel(0, &surface);

	device->SetRenderTarget(0, surface);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0);

	drawcallback(effect);
	surface->Release();

	if( shadowtype == Static )
		needsredraw = false;
}

// *****************************************************************************************************************************
//
// DXPointLight impl
//
// *****************************************************************************************************************************

DXPointLight::DXPointLight(const D3DXCOLOR& color, const D3DXVECTOR3& pos, float radius)
{
	this->color = color;
	this->position = pos;
	this->radius = radius;

	shadowmap = 0;
	blur = 0;
	needsredraw = true;
	needsblur = true;
}

DXPointLight::~DXPointLight()
{
	if( shadowmap )
	{
		shadowmap->Release();
		shadowmap = 0;
	}

	if( blur )
	{
		blur->Release();
		blur = 0;
	}

	if( blurdeclforpoint )
	{
		if( 0 == blurdeclforpoint->Release() )
			blurdeclforpoint = 0;
	}
}

void DXPointLight::CreateShadowMap(LPDIRECT3DDEVICE9 device, DWORD type, DWORD size)
{
	HRESULT hr;

	DXLight::CreateShadowMap(device, type, size);
	hr = device->CreateCubeTexture(size, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &shadowmap, NULL);

	if( SUCCEEDED(hr) )
		hr = device->CreateCubeTexture(size, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &blur, NULL);

	if( FAILED(hr) )
	{
		if( shadowmap )
			shadowmap->Release();

		shadowmap = blur = NULL;
	}

	if( !blurdeclforpoint )
	{
		D3DVERTEXELEMENT9 elem[] =
		{
			{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
			{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			D3DDECL_END()
		};

		device->CreateVertexDeclaration(elem, &blurdeclforpoint);
	}
	else
		blurdeclforpoint->AddRef();
}

void DXPointLight::GetScissorRect(RECT& out, const D3DXMATRIX& view, const D3DXMATRIX& proj, int w, int h) const
{
	// NOTE: should also consider viewport origin

	D3DXVECTOR4 Q;
	D3DXVECTOR3 L, P1, P2;
	float u, v;

	out.left	= 0;
	out.right	= w;
	out.top		= 0;
	out.bottom	= h;

	D3DXVec3TransformCoord(&L, &position, &view);

	float d = 4 * (radius * radius * L.x * L.x - (L.x * L.x + L.z * L.z) * (radius * radius - L.z * L.z));

	if( d >= 0 && L.z >= 0.0f )
	{
		float a1 = (radius * L.x + sqrtf(d * 0.25f)) / (L.x * L.x + L.z * L.z);
		float a2 = (radius * L.x - sqrtf(d * 0.25f)) / (L.x * L.x + L.z * L.z);
		float c1 = (radius - a1 * L.x) / L.z;
		float c2 = (radius - a2 * L.x) / L.z;

		P1.y = 0;
		P2.y = 0;

		P1.x = L.x - a1 * radius;
		P1.z = L.z - c1 * radius;

		P2.x = L.x - a2 * radius;
		P2.z = L.z - c2 * radius;

		if( P1.z > 0 && P2.z > 0 )
		{
			D3DXVec3Transform(&Q, &P1, &proj);

			Q /= Q.w;
			u = (Q.x * 0.5f + 0.5f) * w;

			D3DXVec3Transform(&Q, &P2, &proj);

			Q /= Q.w;
			v = (Q.x * 0.5f + 0.5f) * w;

			if( P2.x < L.x )
			{
				out.left = (LONG)max(v, 0);
				out.right = (LONG)min(u, w);
			}
			else
			{
				out.left = (LONG)max(u, 0);
				out.right = (LONG)min(v, w);
			}
		}
	}

	d = 4 * (radius * radius * L.y * L.y - (L.y * L.y + L.z * L.z) * (radius * radius - L.z * L.z));

	if( d >= 0 && L.z >= 0.0f )
	{
		float b1 = (radius * L.y + sqrtf(d * 0.25f)) / (L.y * L.y + L.z * L.z);
		float b2 = (radius * L.y - sqrtf(d * 0.25f)) / (L.y * L.y + L.z * L.z);
		float c1 = (radius - b1 * L.y) / L.z;
		float c2 = (radius - b2 * L.y) / L.z;

		P1.x = 0;
		P2.x = 0;

		P1.y = L.y - b1 * radius;
		P1.z = L.z - c1 * radius;

		P2.y = L.y - b2 * radius;
		P2.z = L.z - c2 * radius;

		if( P1.z > 0 && P2.z > 0 )
		{
			D3DXVec3Transform(&Q, &P1, &proj);

			Q /= Q.w;
			u = (Q.y * -0.5f + 0.5f) * h;

			D3DXVec3Transform(&Q, &P2, &proj);

			Q /= Q.w;
			v = (Q.y * -0.5f + 0.5f) * h;

			if( P2.y < L.y )
			{
				out.top = (LONG)max(u, 0);
				out.bottom = (LONG)min(v, h);
			}
			else
			{
				out.top = (LONG)max(v, 0);
				out.bottom = (LONG)min(u, h);
			}
		}
	}
}

void DXPointLight::GetViewProjMatrix(D3DXMATRIX& out, DWORD face)
{
	D3DXMATRIX proj;

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 2, 1, 0.1f, radius);
	D3DXMatrixLookAtLH(&out, &position, &(position + DXCubeForward[face]), &DXCubeUp[face]);
	D3DXMatrixMultiply(&out, &out, &proj);
}

void DXPointLight::BlurShadowMap(LPDIRECT3DDEVICE9 device, LPD3DXEFFECT effect)
{
	// not good...

#if 0
	if( !shadowmap || !blur || !blurdeclforpoint || !needsblur )
		return;

	D3DXVECTOR4 texelsizesx[6] =
	{
		D3DXVECTOR4(0, 0, 2.0f / shadowsize, 0),
		D3DXVECTOR4(0, 0, 2.0f / shadowsize, 0),

		D3DXVECTOR4(2.0f / shadowsize, 0, 0, 0),
		D3DXVECTOR4(2.0f / shadowsize, 0, 0, 0),

		D3DXVECTOR4(2.0f / shadowsize, 0, 0, 0),
		D3DXVECTOR4(2.0f / shadowsize, 0, 0, 0),
	};

	D3DXVECTOR4 texelsizesy[6] =
	{
		D3DXVECTOR4(0, 2.0f / shadowsize, 0, 0),
		D3DXVECTOR4(0, 2.0f / shadowsize, 0, 0),

		D3DXVECTOR4(0, 0, 2.0f / shadowsize, 0),
		D3DXVECTOR4(0, 0, 2.0f / shadowsize, 0),

		D3DXVECTOR4(0, 2.0f / shadowsize, 0, 0),
		D3DXVECTOR4(0, 2.0f / shadowsize, 0, 0),
	};

	LPDIRECT3DSURFACE9	surface = NULL;
	D3DXMATRIX			view;
	UINT				stride = 5 * sizeof(float);
	char*				vert = (char*)&DXCubeVertices[0];

	device->SetVertexDeclaration(blurdeclforpoint);

	for( int j = 0; j < 6; ++j )
	{
		D3DXMatrixLookAtLH(&view, &D3DXVECTOR3(0, 0, 0), &DXCubeForward[j], &DXCubeUp[j]);

		// x
		effect->SetMatrix("matViewProj", &view);
		effect->SetVector("texelSize", &texelsizesx[j]);
		effect->CommitChanges();

		blur->GetCubeMapSurface((D3DCUBEMAP_FACES)j, 0, &surface);

		device->SetRenderTarget(0, surface);
		device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

		device->SetTexture(0, shadowmap);
		device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, DXCubeIndices, D3DFMT_INDEX16, vert + j * 4 * stride, stride);

		surface->Release();

		// y
		effect->SetVector("texelSize", &texelsizesy[j]);
		effect->CommitChanges();

		shadowmap->GetCubeMapSurface((D3DCUBEMAP_FACES)j, 0, &surface);

		device->SetRenderTarget(0, surface);
		device->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);

		device->SetTexture(0, blur);
		device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, DXCubeIndices, D3DFMT_INDEX16,  vert + j * 4 * stride, stride);

		surface->Release();
	}

	if( shadowtype == Static )
		needsblur = false;
#endif
}

void DXPointLight::DrawShadowMap(LPDIRECT3DDEVICE9 device, LPD3DXEFFECT effect, void (*drawcallback)(LPD3DXEFFECT))
{
	if( !shadowmap || !needsredraw )
		return;

	LPDIRECT3DSURFACE9 surface = NULL;
	D3DXMATRIX vp;

	effect->SetVector("lightPos", (D3DXVECTOR4*)&position);

	for( int j = 0; j < 6; ++j )
	{
		GetViewProjMatrix(vp, j);
		effect->SetMatrix("matViewProj", &vp);

		shadowmap->GetCubeMapSurface((D3DCUBEMAP_FACES)j, 0, &surface);

		device->SetRenderTarget(0, surface);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0);

		drawcallback(effect);
		surface->Release();
	}

	if( shadowtype == Static )
		needsredraw = false;
}

// *****************************************************************************************************************************
//
// DXSpotLight impl
//
// *****************************************************************************************************************************

DXSpotLight::DXSpotLight(const D3DXCOLOR& color, const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, float inner, float outer, float radius)
{
	this->color			= color;
	this->position		= pos;
	this->direction		= dir;
	
	params.x = cosf(inner);
	params.y = cosf(outer);
	params.z = radius;
}

DXSpotLight::~DXSpotLight()
{
}

void DXSpotLight::CreateShadowMap(LPDIRECT3DDEVICE9 device, DWORD type, DWORD size)
{
	// TODO:
}

void DXSpotLight::GetScissorRect(RECT& out, const D3DXMATRIX& view, const D3DXMATRIX& proj, int w, int h) const
{
	out.left	= 0;
	out.right	= w;
	out.top		= 0;
	out.bottom	= h;

	// TODO:
}

// *****************************************************************************************************************************
//
// Functions impl
//
// *****************************************************************************************************************************

HRESULT DXLoadMeshFromQM(LPCTSTR file, DWORD options, LPDIRECT3DDEVICE9 d3ddevice, D3DXMATERIAL** materials, DWORD* nummaterials, LPD3DXMESH* mesh)
{
	static const unsigned char usages[] =
	{
		D3DDECLUSAGE_POSITION,
		D3DDECLUSAGE_POSITIONT,
		D3DDECLUSAGE_COLOR,
		D3DDECLUSAGE_BLENDWEIGHT,
		D3DDECLUSAGE_BLENDINDICES,
		D3DDECLUSAGE_NORMAL,
		D3DDECLUSAGE_TEXCOORD,
		D3DDECLUSAGE_TANGENT,
		D3DDECLUSAGE_BINORMAL,
		D3DDECLUSAGE_PSIZE,
		D3DDECLUSAGE_TESSFACTOR
	};

	static const unsigned short elemsizes[6] =
	{
		1, 2, 3, 4, 4, 4
	};

	static const unsigned short elemstrides[6] =
	{
		4, 4, 4, 4, 1, 1
	};

	D3DVERTEXELEMENT9*	decl;
	D3DXATTRIBUTERANGE*	table;
	D3DXCOLOR			color;
	HRESULT				hr;
	FILE*				infile = 0;
	unsigned int		unused;
	unsigned int		version;
	unsigned int		numindices;
	unsigned int		numvertices;
	unsigned int		vstride;
	unsigned int		istride;
	unsigned int		numsubsets;
	unsigned int		numelems;
	unsigned short		tmp16;
	unsigned char		tmp8;
	void*				data;
	char				buff[256];

#ifdef _MSC_VER
	fopen_s(&infile, file, "rb");
#else
	infile = fopen(file, "rb")
#endif

	if( !infile )
		return E_FAIL;

	fread(&unused, 4, 1, infile);
	fread(&numindices, 4, 1, infile);
	fread(&istride, 4, 1, infile);
	fread(&numsubsets, 4, 1, infile);

	version = unused >> 16;

	fread(&numvertices, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);

	table = new D3DXATTRIBUTERANGE[numsubsets];

	// vertex declaration
	fread(&numelems, 4, 1, infile);
	decl = new D3DVERTEXELEMENT9[numelems + 1];

	vstride = 0;

	for( unsigned int i = 0; i < numelems; ++i )
	{
		fread(&tmp16, 2, 1, infile);
		decl[i].Stream = tmp16;

		fread(&tmp8, 1, 1, infile);
		decl[i].Usage = usages[tmp8];

		fread(&tmp8, 1, 1, infile);
		decl[i].Type = tmp8;

		fread(&tmp8, 1, 1, infile);
		decl[i].UsageIndex = tmp8;

		decl[i].Method = D3DDECLMETHOD_DEFAULT;
		decl[i].Offset = vstride;

		vstride += elemsizes[decl[i].Type] * elemstrides[decl[i].Type];
	}

	decl[numelems].Stream = 0xff;
	decl[numelems].Offset = 0;
	decl[numelems].Type = D3DDECLTYPE_UNUSED;
	decl[numelems].Method = 0;
	decl[numelems].Usage = 0;
	decl[numelems].UsageIndex = 0;

	// create mesh
	if( istride == 4 )
		options |= D3DXMESH_32BIT;

	hr = D3DXCreateMesh(numindices / 3, numvertices, options, decl, d3ddevice, mesh);

	if( FAILED(hr) )
		goto _fail;

	(*mesh)->LockVertexBuffer(0, &data);
	fread(data, vstride, numvertices, infile);
	(*mesh)->UnlockVertexBuffer();

	(*mesh)->LockIndexBuffer(0, &data);
	fread(data, istride, numindices, infile);
	(*mesh)->UnlockIndexBuffer();

	if( version >= 1 )
	{
		fread(&unused, 4, 1, infile);

		if( unused > 0 )
			fseek(infile, 8 * unused, SEEK_CUR);
	}

	// attribute table
	(*materials) = new D3DXMATERIAL[numsubsets];

	for( unsigned int i = 0; i < numsubsets; ++i )
	{
		D3DXATTRIBUTERANGE& subset = table[i];
		D3DXMATERIAL& mat = (*materials)[i];

		mat.pTextureFilename = 0;
		subset.AttribId = i;

		fread(&subset.FaceStart, 4, 1, infile);
		fread(&subset.VertexStart, 4, 1, infile);
		fread(&subset.VertexCount, 4, 1, infile);
		fread(&subset.FaceCount, 4, 1, infile);

		subset.FaceCount /= 3;
		subset.FaceStart /= 3;

		fseek(infile, 6 * sizeof(float), SEEK_CUR);

		DXReadString(infile, buff);
		DXReadString(infile, buff);

		bool hasmaterial = (buff[1] != ',');

		if( hasmaterial )
		{
			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			mat.MatD3D.Ambient = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			mat.MatD3D.Diffuse = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			mat.MatD3D.Specular = (D3DCOLORVALUE&)color;

			fread(&color, sizeof(D3DXCOLOR), 1, infile);
			mat.MatD3D.Emissive = (D3DCOLORVALUE&)color;

			fread(&mat.MatD3D.Power, sizeof(float), 1, infile);
			fread(&mat.MatD3D.Diffuse.a, sizeof(float), 1, infile);

			fread(&unused, 4, 1, infile);
			DXReadString(infile, buff);

			if( buff[1] != ',' )
			{
				unused = strlen(buff);

				mat.pTextureFilename = new char[unused + 1];
				memcpy(mat.pTextureFilename, buff, unused);
				mat.pTextureFilename[unused] = 0;
			}

			DXReadString(infile, buff);
			DXReadString(infile, buff);
			DXReadString(infile, buff);
			DXReadString(infile, buff);
			DXReadString(infile, buff);
			DXReadString(infile, buff);
			DXReadString(infile, buff);
		}
		else
		{
			color = D3DXCOLOR(1, 1, 1, 1);

			memcpy(&mat.MatD3D.Ambient, &color, 4 * sizeof(float));
			memcpy(&mat.MatD3D.Diffuse, &color, 4 * sizeof(float));
			memcpy(&mat.MatD3D.Specular, &color, 4 * sizeof(float));

			color = D3DXCOLOR(0, 0, 0, 1);
			memcpy(&mat.MatD3D.Emissive, &color, 4 * sizeof(float));

			mat.MatD3D.Power = 80.0f;
		}

		DXReadString(infile, buff);

		if( buff[1] != ',' && mat.pTextureFilename == 0 )
		{
			unused = strlen(buff);

			mat.pTextureFilename = new char[unused + 1];
			memcpy(mat.pTextureFilename, buff, unused);
			mat.pTextureFilename[unused] = 0;
		}

		DXReadString(infile, buff);
		DXReadString(infile, buff);
		DXReadString(infile, buff);
		DXReadString(infile, buff);
		DXReadString(infile, buff);
		DXReadString(infile, buff);
		DXReadString(infile, buff);
	}

	// attribute buffer
	(*mesh)->LockAttributeBuffer(0, (DWORD**)&data);

	for( unsigned int i = 0; i < numsubsets; ++i )
	{
		const D3DXATTRIBUTERANGE& subset = table[i];

		for( DWORD j = 0; j < subset.FaceCount; ++j )
			*((DWORD*)data + subset.FaceStart + j) = subset.AttribId;
	}

	(*mesh)->UnlockAttributeBuffer();
	(*mesh)->SetAttributeTable(table, numsubsets);

	*nummaterials = numsubsets;

_fail:
	delete[] decl;
	delete[] table;

	fclose(infile);
	return hr;
}

HRESULT DXSaveMeshToQM(LPCTSTR file, LPD3DXMESH mesh, D3DXMATERIAL* materials, DWORD nummaterials)
{
	static const unsigned char usages[] =
	{
		0,		// D3DDECLUSAGE_POSITION
		3,		// D3DDECLUSAGE_BLENDWEIGHT
		4,		// D3DDECLUSAGE_BLENDINDICES
		5,		// D3DDECLUSAGE_NORMAL
		9,		// D3DDECLUSAGE_PSIZE
		6,		// D3DDECLUSAGE_TEXCOORD
		7,		// D3DDECLUSAGE_TANGENT
		8,		// D3DDECLUSAGE_BINORMAL
		10,		// D3DDECLUSAGE_TESSFACTOR
		1,		// D3DDECLUSAGE_POSITIONT
		2,		// D3DDECLUSAGE_COLOR
		0xff,	// D3DDECLUSAGE_FOG
		0xff,	// D3DDECLUSAGE_DEPTH
		0xff,	// D3DDECLUSAGE_SAMPLE
	};

	D3DVERTEXELEMENT9*		decl;
	D3DXATTRIBUTERANGE*		table = 0;
	DWORD					tablesize = 0;
	DWORD					declsize;
	FILE*					outfile = 0;
	void*					data = 0;

#ifdef _MSC_VER
	fopen_s(&outfile, file, "wb");
#else
	outfile = fopen(file, "wb");
#endif

	if( !outfile )
		return E_FAIL;

	mesh->GetAttributeTable(NULL, &tablesize);
	
	if( tablesize > 0 )
	{
		table = new D3DXATTRIBUTERANGE[tablesize];
		mesh->GetAttributeTable(table, &tablesize);
	}

	decl = new D3DVERTEXELEMENT9[MAX_FVF_DECL_SIZE];
	mesh->GetDeclaration(decl);

	for( declsize = 0; declsize < MAX_FVF_DECL_SIZE; ++declsize )
	{
		if( decl[declsize].Stream == 0xff )
			break;
	}

	// header
	unsigned int	unused = 0;
	unsigned int	numvertices		= mesh->GetNumVertices();
	unsigned int	numindices		= mesh->GetNumFaces() * 3;
	unsigned int	istride			= ((mesh->GetOptions() & D3DXMESH_32BIT) ? 4 : 2);
	unsigned short	tmp16;
	unsigned char	tmp8;

	if( tablesize == 0 )
	{
		tablesize = 1;
		table = new D3DXATTRIBUTERANGE();

		table->AttribId = 0;
		table->FaceCount = numindices / 3;
		table->FaceStart = 0;
		table->VertexCount = numvertices;
		table->VertexStart = 0;
	}

	fwrite(&unused, 4, 1, outfile);
	fwrite(&numindices, 4, 1, outfile);
	fwrite(&istride, 4, 1, outfile);
	fwrite(&tablesize, 4, 1, outfile);

	unused = 0;

	fwrite(&numvertices, 4, 1, outfile);
	fwrite(&unused, 4, 1, outfile);
	fwrite(&unused, 4, 1, outfile);
	fwrite(&unused, 4, 1, outfile);

	// declaration
	fwrite(&declsize, 4, 1, outfile);

	for( DWORD i = 0; i < declsize; ++i )
	{
		tmp16 = decl[i].Stream;

		fwrite(&tmp16, 2, 1, outfile);
		fwrite(&usages[decl[i].Usage], 1, 1, outfile);

		tmp8 = decl[i].Type;
		fwrite(&tmp8, 1, 1, outfile);

		tmp8 = decl[i].UsageIndex;
		fwrite(&tmp8, 1, 1, outfile);
	}

	// vertex data
	mesh->LockVertexBuffer(D3DLOCK_READONLY, &data);
	fwrite(data, mesh->GetNumBytesPerVertex(), numvertices, outfile);
	mesh->UnlockVertexBuffer();

	mesh->LockIndexBuffer(D3DLOCK_READONLY, &data);
	fwrite(data, istride, numindices, outfile);
	mesh->UnlockIndexBuffer();

	// subset table
	D3DXVECTOR3 bbmin(FLT_MAX, FLT_MAX, FLT_MAX);
	D3DXVECTOR3 bbmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	D3DXCOLOR color;
	int blendmode;

	for( DWORD i = 0; i < tablesize; ++i )
	{
		unused = table[i].FaceStart * 3;
		fwrite(&unused, 4, 1, outfile);

		fwrite(&table[i].VertexStart, 4, 1, outfile);
		fwrite(&table[i].VertexCount, 4, 1, outfile);

		unused = table[i].FaceCount * 3;
		fwrite(&unused, 4, 1, outfile);

		fwrite(&bbmin, sizeof(D3DXVECTOR3), 1, outfile);
		fwrite(&bbmax, sizeof(D3DXVECTOR3), 1, outfile);

		fprintf(outfile, "unnamed%lu\n", i);

		// material
		if( table[i].AttribId < nummaterials )
		{
			const D3DXMATERIAL& mat = materials[table[i].AttribId];

			fprintf(outfile, "unnamed%lu\n", table[i].AttribId);

			color = mat.MatD3D.Ambient;
			fwrite(&color, sizeof(D3DXCOLOR), 1, outfile);

			color = mat.MatD3D.Diffuse;
			fwrite(&color, sizeof(D3DXCOLOR), 1, outfile);

			color = mat.MatD3D.Specular;
			fwrite(&color, sizeof(D3DXCOLOR), 1, outfile);

			color = mat.MatD3D.Emissive;
			fwrite(&color, sizeof(D3DXCOLOR), 1, outfile);

			fwrite(&mat.MatD3D.Power, sizeof(float), 1, outfile);
			fwrite(&mat.MatD3D.Diffuse.a, sizeof(float), 1, outfile);

			blendmode = ((mat.MatD3D.Diffuse.a < 1.0f) ? 1 : 0);
			fwrite(&blendmode, sizeof(int), 1, outfile);

			fprintf(outfile, ",,\n,,\n,,\n,,\n,,\n,,\n,,\n,,\n");

			if( mat.pTextureFilename )
				fprintf(outfile, "%s\n", mat.pTextureFilename);
			else
				fprintf(outfile, ",,\n");

			fprintf(outfile, ",,\n,,\n,,\n,,\n,,\n,,\n,,\n");
		}
		else
			fprintf(outfile, ",,\n,,\n,,\n,,\n,,\n,,\n,,\n,,\n,,\n");
	}

	delete[] table;
	delete[] decl;

	fclose(outfile);
	return S_OK;
}

HRESULT DXCreateEffect(LPCTSTR file, LPDIRECT3DDEVICE9 d3ddevice, LPD3DXEFFECT* out)
{
	HRESULT hr;
	LPD3DXBUFFER errors = NULL;

	if( FAILED(hr = D3DXCreateEffectFromFileA(d3ddevice, file, NULL, NULL, D3DXSHADER_DEBUG, NULL, out, &errors)) )
	{
		if( errors )
		{
			char* str = (char*)errors->GetBufferPointer();
			std::cout << str << "\n\n";
		}
	}

	if( errors )
		errors->Release();

	return hr;
}

HRESULT DXGenTangentFrame(LPDIRECT3DDEVICE9 d3ddevice, LPD3DXMESH* mesh)
{
	LPD3DXMESH newmesh = NULL;
	HRESULT hr;

	D3DVERTEXELEMENT9 decl[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 32, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0 },
		{ 0, 44, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
		D3DDECL_END()
	};

	hr = (*mesh)->CloneMesh(D3DXMESH_MANAGED, decl, d3ddevice, &newmesh);

	if( FAILED(hr) )
		return hr;

	(*mesh)->Release();
	(*mesh) = NULL;

	hr = D3DXComputeTangentFrameEx(newmesh, D3DDECLUSAGE_TEXCOORD, 0,
		D3DDECLUSAGE_TANGENT, 0, D3DDECLUSAGE_BINORMAL, 0, D3DDECLUSAGE_NORMAL, 0,
		0, NULL, 0.01f, 0.25f, 0.01f, mesh, NULL);

	newmesh->Release();
	return hr;
}

void DXRenderText(const std::string& str, LPDIRECT3DTEXTURE9 tex, DWORD width, DWORD height)
{
	if( tex == 0 )
		return;

	if( gdiplustoken == 0 )
	{
		Gdiplus::GdiplusStartupInput gdiplustartup;
		Gdiplus::GdiplusStartup(&gdiplustoken, &gdiplustartup, NULL);
	}

	Gdiplus::Color				outline(0xff000000);
	Gdiplus::Color				fill(0xffffffff);

	Gdiplus::Bitmap*			bitmap;
	Gdiplus::Graphics*			graphics;
	Gdiplus::GraphicsPath		path;
	Gdiplus::FontFamily			family(L"Arial");
	Gdiplus::StringFormat		format;
	Gdiplus::Pen				pen(outline, 3);
	Gdiplus::SolidBrush			brush(fill);
	std::wstring				wstr(str.begin(), str.end());

	//format.SetAlignment(Gdiplus::StringAlignmentFar);

	bitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
	graphics = new Gdiplus::Graphics(bitmap);

	// render text
	graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	graphics->SetPageUnit(Gdiplus::UnitPixel);

	path.AddString(wstr.c_str(), wstr.length(), &family, Gdiplus::FontStyleBold, 25, Gdiplus::Point(0, 0), &format);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	graphics->DrawPath(&pen, &path);
	graphics->FillPath(&brush, &path);

	// copy to texture
	Gdiplus::Rect rc(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	Gdiplus::BitmapData data;
	D3DLOCKED_RECT d3drc;

	memset(&data, 0, sizeof(Gdiplus::BitmapData));

	bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
	tex->LockRect(0, &d3drc, 0, 0);

	memcpy(d3drc.pBits, data.Scan0, width * height * 4);

	tex->UnlockRect(0);
	bitmap->UnlockBits(&data);

	delete graphics;
	delete bitmap;
}

void DXGetCubemapViewMatrix(D3DXMATRIX& out, DWORD i, const D3DXVECTOR3& eye)
{
	D3DXVECTOR3 look = eye + DXCubeForward[i];
	D3DXMatrixLookAtLH(&out, &eye, &look, &DXCubeUp[i]);
}

void DXFitToBox(D3DXMATRIX& out, D3DXVECTOR4& clipout, const D3DXMATRIX& view, const DXAABox& box)
{
	D3DXMATRIX		transp;
	D3DXVECTOR4		pleft(1, 0, 0, 0);
	D3DXVECTOR4		pbottom(0, 1, 0, 0);
	D3DXVECTOR4		pnear(0, 0, 1, 0);

	D3DXMatrixTranspose(&transp, &view);

	D3DXVec4Transform(&pleft, &pleft, &transp);
	D3DXVec4Transform(&pbottom, &pbottom, &transp);
	D3DXVec4Transform(&pnear, &pnear, &transp);

	float left = box.Nearest((const D3DXPLANE&)pleft);
	float right = box.Farthest((const D3DXPLANE&)pleft);
	float bottom = box.Nearest((const D3DXPLANE&)pbottom);
	float top = box.Farthest((const D3DXPLANE&)pbottom);

	clipout.x = box.Nearest((const D3DXPLANE&)pnear);
	clipout.y = box.Farthest((const D3DXPLANE&)pnear);
	clipout.z = right - left;
	clipout.w = top - bottom;

	D3DXMatrixOrthoOffCenterLH(&out, left, right, bottom, top, clipout.x, clipout.y);
}

HRESULT DXCreateTexturedBox(LPDIRECT3DDEVICE9 d3ddevice, DWORD options, LPD3DXMESH* out)
{
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	LPD3DXMESH			mesh	= 0;
	DXTexturedVertex*	vdata	= 0;
	WORD*				idata	= 0;
	DWORD*				adata	= 0;
	HRESULT				hr		= D3DXCreateMesh(12, 24, options, elem, d3ddevice, &mesh);

	if( FAILED(hr) )
		return hr;

	mesh->LockVertexBuffer(0, (void**)&vdata);
	mesh->LockIndexBuffer(0, (void**)&idata);
	mesh->LockAttributeBuffer(0, &adata);

	// Z- face
	vdata[0].x = -0.5f;		vdata[1].x = -0.5f;		vdata[2].x = 0.5f;		vdata[3].x = 0.5f;
	vdata[0].y = -0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = -0.5f;
	vdata[0].z = -0.5f;		vdata[1].z = -0.5f;		vdata[2].z = -0.5f;		vdata[3].z = -0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = 0;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = 0;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = -1;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 1;	vdata[3].u = 1;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;	vdata[3].v = 1;

	vdata += 4;

	// X- face
	vdata[0].x = -0.5f;		vdata[1].x = -0.5f;		vdata[2].x = -0.5f;		vdata[3].x = -0.5f;
	vdata[0].y = -0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = -0.5f;
	vdata[0].z = 0.5f;		vdata[1].z = 0.5f;		vdata[2].z = -0.5f;		vdata[3].z = -0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = -1;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = 0;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = 0;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 1;	vdata[3].u = 1;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;	vdata[3].v = 1;

	vdata += 4;

	// X+ face
	vdata[0].x = 0.5f;		vdata[1].x = 0.5f;		vdata[2].x = 0.5f;		vdata[3].x = 0.5f;
	vdata[0].y = -0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = -0.5f;
	vdata[0].z = -0.5f;		vdata[1].z = -0.5f;		vdata[2].z = 0.5f;		vdata[3].z = 0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = 1;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = 0;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = 0;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 1;	vdata[3].u = 1;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;	vdata[3].v = 1;

	vdata += 4;

	// Z+ face
	vdata[0].x = 0.5f;		vdata[1].x = 0.5f;		vdata[2].x = -0.5f;		vdata[3].x = -0.5f;
	vdata[0].y = -0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = -0.5f;
	vdata[0].z = 0.5f;		vdata[1].z = 0.5f;		vdata[2].z = 0.5f;		vdata[3].z = 0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = 0;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = 0;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = 1;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 1;	vdata[3].u = 1;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;	vdata[3].v = 1;

	vdata += 4;

	// Y+ face
	vdata[0].x = -0.5f;		vdata[1].x = -0.5f;		vdata[2].x = 0.5f;		vdata[3].x = 0.5f;
	vdata[0].y = 0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = 0.5f;
	vdata[0].z = -0.5f;		vdata[1].z = 0.5f;		vdata[2].z = 0.5f;		vdata[3].z = -0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = 0;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = 1;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = 0;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 1;	vdata[3].u = 1;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;	vdata[3].v = 1;

	vdata += 4;

	// Y- face
	vdata[0].x = -0.5f;		vdata[1].x = -0.5f;		vdata[2].x = 0.5f;		vdata[3].x = 0.5f;
	vdata[0].y = -0.5f;		vdata[1].y = -0.5f;		vdata[2].y = -0.5f;		vdata[3].y = -0.5f;
	vdata[0].z = 0.5f;		vdata[1].z = -0.5f;		vdata[2].z = -0.5f;		vdata[3].z = 0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = 0;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = -1;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = 0;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 1;	vdata[3].u = 1;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;	vdata[3].v = 1;

	for( int i = 0; i < 6; ++i )
	{
		idata[0] = idata[3]		= i * 4;
		idata[1]				= i * 4 + 1;
		idata[2] = idata[4]		= i * 4 + 2;
		idata[5]				= i * 4 + 3;

		idata += 6;
	}

	memset(adata, 0, 12 * sizeof(DWORD));

	mesh->UnlockAttributeBuffer();
	mesh->UnlockIndexBuffer();
	mesh->UnlockVertexBuffer();

	*out = mesh;
	return D3D_OK;
}

HRESULT DXCreateCollisionBox(LPDIRECT3DDEVICE9 d3ddevice, DWORD options, LPD3DXMESH* out)
{
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		D3DDECL_END()
	};

	LPD3DXMESH			mesh	= 0;
	DXCollisionVertex*	vdata	= 0;
	WORD*				idata	= 0;
	DWORD*				adata	= 0;
	HRESULT				hr		= D3DXCreateMesh(12, 8, options, elem, d3ddevice, &mesh);

	if( FAILED(hr) )
		return hr;

	mesh->LockVertexBuffer(0, (void**)&vdata);
	mesh->LockIndexBuffer(0, (void**)&idata);
	mesh->LockAttributeBuffer(0, &adata);

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

	mesh->UnlockAttributeBuffer();
	mesh->UnlockIndexBuffer();
	mesh->UnlockVertexBuffer();

	*out = mesh;
	return D3D_OK;
}

HRESULT DXCreateTexturedLShape(LPDIRECT3DDEVICE9 d3ddevice, DWORD options, LPD3DXMESH* out)
{
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	LPD3DXMESH			mesh	= 0;
	DXTexturedVertex*	vdata	= 0;
	WORD*				idata	= 0;
	DWORD*				adata	= 0;
	HRESULT				hr		= D3DXCreateMesh(20, 36, options, elem, d3ddevice, &mesh);

	if( FAILED(hr) )
		return hr;

	mesh->LockVertexBuffer(0, (void**)&vdata);
	mesh->LockIndexBuffer(0, (void**)&idata);
	mesh->LockAttributeBuffer(0, &adata);

	// Z- face
	vdata[0].x = -0.5f;		vdata[1].x = -0.5f;		vdata[2].x = 0.5f;		vdata[3].x = 0.5f;
	vdata[0].y = -0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = -0.5f;
	vdata[0].z = -0.5f;		vdata[1].z = -0.5f;		vdata[2].z = -0.5f;		vdata[3].z = -0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = 0;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = 0;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = -1;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 1;	vdata[3].u = 1;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;	vdata[3].v = 1;

	vdata += 4;

	// X- face
	vdata[0].x = -0.5f;		vdata[1].x = -0.5f;		vdata[2].x = -0.5f;		vdata[3].x = -0.5f;
	vdata[0].y = -0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = -0.5f;
	vdata[0].z = 0.5f;		vdata[1].z = 0.5f;		vdata[2].z = -0.5f;		vdata[3].z = -0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = -1;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = 0;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = 0;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 1;	vdata[3].u = 1;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;	vdata[3].v = 1;

	vdata += 4;

	// X+ faces
	vdata[0].x = 0;			vdata[1].x = 0;			vdata[2].x = 0;			vdata[3].x = 0;
	vdata[0].y = -0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = -0.5f;
	vdata[0].z = 0;			vdata[1].z = 0;			vdata[2].z = 0.5f;		vdata[3].z = 0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = 1;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = 0;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = 0;

	vdata[0].u = 0.5f;	vdata[1].u = 0.5f;	vdata[2].u = 1;	vdata[3].u = 1;
	vdata[0].v = 1;		vdata[1].v = 0;		vdata[2].v = 0;	vdata[3].v = 1;

	vdata += 4;

	vdata[0].x = 0.5f;		vdata[1].x = 0.5f;		vdata[2].x = 0.5f;		vdata[3].x = 0.5f;
	vdata[0].y = -0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = -0.5f;
	vdata[0].z = -0.5f;		vdata[1].z = -0.5f;		vdata[2].z = 0;			vdata[3].z = 0;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = 1;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = 0;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = 0;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 0.5f;	vdata[3].u = 0.5f;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;		vdata[3].v = 1;

	vdata += 4;

	// Z+ faces
	vdata[0].x = 0;			vdata[1].x = 0;			vdata[2].x = -0.5f;		vdata[3].x = -0.5f;
	vdata[0].y = -0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = -0.5f;
	vdata[0].z = 0.5f;		vdata[1].z = 0.5f;		vdata[2].z = 0.5f;		vdata[3].z = 0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = 0;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = 0;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = 1;

	vdata[0].u = 0.5f;	vdata[1].u = 0.5f;	vdata[2].u = 1;	vdata[3].u = 1;
	vdata[0].v = 1;		vdata[1].v = 0;		vdata[2].v = 0;	vdata[3].v = 1;

	vdata += 4;

	vdata[0].x = 0.5f;		vdata[1].x = 0.5f;		vdata[2].x = 0;			vdata[3].x = 0;
	vdata[0].y = -0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = -0.5f;
	vdata[0].z = 0;			vdata[1].z = 0;			vdata[2].z = 0;			vdata[3].z = 0;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = 0;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = 0;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = 1;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 0.5f;	vdata[3].u = 0.5f;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;		vdata[3].v = 1;

	vdata += 4;

	// Y+ face
	vdata[0].x = -0.5f;		vdata[1].x = -0.5f;		vdata[2].x = 0;			vdata[3].x = 0;			vdata[4].x = 0.5f;		vdata[5].x = 0.5f;
	vdata[0].y = 0.5f;		vdata[1].y = 0.5f;		vdata[2].y = 0.5f;		vdata[3].y = 0.5f;		vdata[4].y = 0.5f;		vdata[5].y = 0.5f;
	vdata[0].z = -0.5f;		vdata[1].z = 0.5f;		vdata[2].z = 0.5f;		vdata[3].z = 0;			vdata[4].z = 0;			vdata[5].z = -0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = vdata[4].nx = vdata[5].nx = 0;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = vdata[4].ny = vdata[5].ny = 1;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = vdata[4].nz = vdata[5].nz = 0;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 0.5f;	vdata[3].u = 0.5f;	vdata[4].u = 1;		vdata[5].u = 1;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;		vdata[3].v = 0.5f;	vdata[4].v = 0.5f;	vdata[5].v = 1;

	vdata += 6;

	// Y- face
	vdata[0].x = -0.5f;		vdata[1].x = -0.5f;		vdata[2].x = 0.5f;		vdata[3].x = 0.5f;		vdata[4].x = 0;			vdata[5].x = 0;
	vdata[0].y = -0.5f;		vdata[1].y = -0.5f;		vdata[2].y = -0.5f;		vdata[3].y = -0.5f;		vdata[4].y = -0.5f;		vdata[5].y = -0.5f;
	vdata[0].z = 0.5f;		vdata[1].z = -0.5f;		vdata[2].z = -0.5f;		vdata[3].z = 0;			vdata[4].z = 0;			vdata[5].z = 0.5f;

	vdata[0].nx = vdata[1].nx = vdata[2].nx = vdata[3].nx = vdata[4].nx = vdata[5].nx = 0;
	vdata[0].ny = vdata[1].ny = vdata[2].ny = vdata[3].ny = vdata[4].ny = vdata[5].ny = -1;
	vdata[0].nz = vdata[1].nz = vdata[2].nz = vdata[3].nz = vdata[4].nz = vdata[5].nz = 0;

	vdata[0].u = 0;	vdata[1].u = 0;	vdata[2].u = 1;	vdata[3].u = 1;		vdata[4].u = 0.5f;	vdata[5].u = 0.5f;
	vdata[0].v = 1;	vdata[1].v = 0;	vdata[2].v = 0;	vdata[3].v = 0.5f;	vdata[4].v = 0.5f;	vdata[5].v = 1;

	for( int i = 0; i < 6; ++i )
	{
		idata[0] = idata[3]		= i * 4;
		idata[1]				= i * 4 + 1;
		idata[2] = idata[4]		= i * 4 + 2;
		idata[5]				= i * 4 + 3;

		idata += 6;
	}

	idata[0] = idata[3] = idata[6] = idata[9] = 24;
	idata[1] = 25;
	idata[2] = idata[4] = 26;
	idata[5] = idata[7] = 27;
	idata[8] = idata[10] = 28;
	idata[11] = 29;

	idata += 12;

	idata[0] = idata[3] = idata[6] = idata[9] = 31;
	idata[1] = 32;
	idata[2] = idata[4] = 33;
	idata[5] = idata[7] = 34;
	idata[8] = idata[10] = 35;
	idata[11] = 30;

	memset(adata, 0, 12 * sizeof(DWORD));

	mesh->UnlockAttributeBuffer();
	mesh->UnlockIndexBuffer();
	mesh->UnlockVertexBuffer();

	*out = mesh;
	return D3D_OK;
}

HRESULT DXCreateCollisionLShape(LPDIRECT3DDEVICE9 d3ddevice, DWORD options, LPD3DXMESH* out)
{
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		D3DDECL_END()
	};

	LPD3DXMESH			mesh	= 0;
	DXCollisionVertex*	vdata	= 0;
	WORD*				idata	= 0;
	DWORD*				adata	= 0;
	HRESULT				hr		= D3DXCreateMesh(20, 12, options, elem, d3ddevice, &mesh);

	if( FAILED(hr) )
		return hr;

	mesh->LockVertexBuffer(0, (void**)&vdata);
	mesh->LockIndexBuffer(0, (void**)&idata);
	mesh->LockAttributeBuffer(0, &adata);

	vdata[0].x = -0.5f;		vdata[1].x = -0.5f;		vdata[2].x = -0.5f;		vdata[3].x = -0.5f;
	vdata[0].y = -0.5f;		vdata[1].y = -0.5f;		vdata[2].y = 0.5f;		vdata[3].y = 0.5f;
	vdata[0].z = -0.5f;		vdata[1].z = 0.5f;		vdata[2].z = -0.5f;		vdata[3].z = 0.5f;

	vdata[4].x = 0.5f;		vdata[5].x = 0.5f;		vdata[6].x = 0.5f;		vdata[7].x = 0.5f;
	vdata[4].y = -0.5f;		vdata[5].y = -0.5f;		vdata[6].y = 0.5f;		vdata[7].y = 0.5f;
	vdata[4].z = -0.5f;		vdata[5].z = 0;			vdata[6].z = -0.5f;		vdata[7].z = 0;

	vdata[8].x = 0;			vdata[9].x = 0;			vdata[10].x = 0;		vdata[11].x = 0;
	vdata[8].y = -0.5f;		vdata[9].y = -0.5f;		vdata[10].y = 0.5f;		vdata[11].y = 0.5f;
	vdata[8].z = 0;			vdata[9].z = 0.5f;		vdata[10].z = 0;		vdata[11].z = 0.5f;

	// you dont have to understand...
	idata[0] = idata[3] = idata[11] = idata[48] = idata[51] = idata[54] = idata[57]		= 0;
	idata[1] = idata[8] = idata[10] = idata[36] = idata[39] = idata[42] = idata[45]		= 2;
	idata[2] = idata[4] = idata[13] = idata[47]											= 6;
	idata[5] = idata[12] = idata[15] = idata[49]										= 4;
	idata[6] = idata[9] = idata[35] = idata[59]											= 1;
	idata[7] = idata[32] = idata[34] = idata[37]										= 3;
	idata[14] = idata[16] = idata[19] = idata[44] = idata[46]							= 7;
	idata[17] = idata[18] = idata[21] = idata[50] = idata[52]							= 5;
	idata[20] = idata[22] = idata[25] = idata[41] = idata[43]							= 10;
	idata[23] = idata[24] = idata[27] = idata[53] = idata[55]							= 8;
	idata[26] = idata[28] = idata[31] = idata[38] = idata[40]							= 11;
	idata[29] = idata[30] = idata[33] = idata[56] = idata[58]							= 9;

	memset(adata, 0, 12 * sizeof(DWORD));

	mesh->UnlockAttributeBuffer();
	mesh->UnlockIndexBuffer();
	mesh->UnlockVertexBuffer();

	*out = mesh;
	return D3D_OK;
}

HRESULT DXCreateTexturedSphere(LPDIRECT3DDEVICE9 d3ddevice, float radius, UINT slices, UINT stacks, DWORD options, LPD3DXMESH* out)
{
	// NOTE: this is not correct, must duplicate top and bottom vertices 'slices' times

	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	LPD3DXMESH			mesh	= 0;
	DXTexturedVertex*	vdata	= 0;
	WORD*				idata	= 0;
	DWORD*				adata	= 0;
	HRESULT				hr;
	float				theta, phi;

	if( slices < 4 )
		slices = 4;

	if( stacks < 4 )
		stacks = 4;

	UINT numvertices = (stacks - 2) * slices + 2;
	UINT numindices = (stacks - 2) * (slices - 1) * 6;

	hr = D3DXCreateMesh(numindices / 3, numvertices, options, elem, d3ddevice, &mesh);

	if( FAILED(hr) )
		return hr;

	mesh->LockVertexBuffer(0, (void**)&vdata);
	mesh->LockIndexBuffer(0, (void**)&idata);
	mesh->LockAttributeBuffer(0, &adata);

	// generate vertices
	for( UINT j = 1; j < stacks - 1; ++j )
	{
		for( UINT i = 0; i < slices; ++i )
		{
			theta = ((float)j / (float)(stacks - 1)) * D3DX_PI;
			phi = ((float)i / (float)(slices - 1)) * D3DX_PI * 2;

			vdata->x = sinf(theta) * cosf(phi) * radius;
			vdata->y = cosf(theta) * radius;
			vdata->z = -sinf(theta) * sinf(phi) * radius;

			vdata->nx = vdata->x / radius;
			vdata->ny = vdata->y / radius;
			vdata->nz = vdata->z / radius;

			vdata->u = phi / (D3DX_PI * 2);
			vdata->v = theta / D3DX_PI;

			++vdata;
		}
	}

	// top
	vdata->x = vdata->z = 0;
	vdata->y = radius;

	vdata->nx = vdata->nz = 0;
	vdata->ny = 1;

	vdata->u = 0.5f;
	vdata->v = 1;

	++vdata;

	// bottom
	vdata->x = vdata->z = 0;
	vdata->y = -radius;

	vdata->nx = vdata->nz = 0;
	vdata->ny = -1;

	vdata->u = 0.5f;
	vdata->v = 0;

	// generate indices
	for( UINT j = 0; j < stacks - 3; ++j )
	{
		for( UINT i = 0; i < slices - 1; ++i )
		{
			idata[0] = j * slices + i;
			idata[1] = (j + 1) * slices + i + 1;
			idata[2] = j * slices + i + 1;

			idata[3] = j * slices + i;
			idata[4] = (j + 1) * slices + i;
			idata[5] = (j + 1) * slices + i + 1;

			idata += 6;
		}
	}

	for( UINT i = 0; i < slices - 1; ++i )
	{
		idata[0] = (stacks - 2) * slices; // top
		idata[1] = i;
		idata[2] = i + 1;

		idata[3] = (stacks - 2) * slices + 1; // bottom
		idata[4] = (stacks - 3) * slices + i + 1;
		idata[5] = (stacks - 3) * slices + i;

		idata += 6;
	}

	memset(adata, 0, 12 * sizeof(DWORD));

	mesh->UnlockAttributeBuffer();
	mesh->UnlockIndexBuffer();
	mesh->UnlockVertexBuffer();

	*out = mesh;
	return D3D_OK;
}

HRESULT DXCreateCollisionSphere(LPDIRECT3DDEVICE9 d3ddevice, float radius, UINT slices, UINT stacks, DWORD options, LPD3DXMESH* out)
{
	D3DVERTEXELEMENT9 elem[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		D3DDECL_END()
	};

	LPD3DXMESH			mesh	= 0;
	DXCollisionVertex*	vdata	= 0;
	WORD*				idata	= 0;
	DWORD*				adata	= 0;
	HRESULT				hr;
	float				theta, phi;

	if( slices < 4 )
		slices = 4;

	if( stacks < 4 )
		stacks = 4;

	UINT numvertices = (stacks - 2) * slices + 2;
	UINT numindices = (stacks - 2) * slices * 6;

	hr = D3DXCreateMesh(numindices / 3, numvertices, options, elem, d3ddevice, &mesh);

	if( FAILED(hr) )
		return hr;

	mesh->LockVertexBuffer(0, (void**)&vdata);
	mesh->LockIndexBuffer(0, (void**)&idata);
	mesh->LockAttributeBuffer(0, &adata);

	// generate vertices
	for( UINT j = 1; j < stacks - 1; ++j )
	{
		for( UINT i = 0; i < slices; ++i )
		{
			theta = ((float)j / (float)(stacks - 1)) * D3DX_PI;
			phi = ((float)i / (float)slices) * D3DX_PI * 2;

			vdata->x = sinf(theta) * cosf(phi) * radius;
			vdata->y = cosf(theta) * radius;
			vdata->z = -sinf(theta) * sinf(phi) * radius;

			++vdata;
		}
	}

	// top
	vdata->x = vdata->z = 0;
	vdata->y = radius;

	++vdata;

	// bottom
	vdata->x = vdata->z = 0;
	vdata->y = -radius;

	// generate indices
	for( UINT j = 0; j < stacks - 3; ++j )
	{
		for( UINT i = 0; i < slices; ++i )
		{
			idata[0] = j * slices + i;
			idata[1] = (j + 1) * slices + (i + 1) % slices;
			idata[2] = j * slices + (i + 1) % slices;

			idata[3] = j * slices + i;
			idata[4] = (j + 1) * slices + i;
			idata[5] = (j + 1) * slices + (i + 1) % slices;

			idata += 6;
		}
	}

	for( UINT i = 0; i < slices; ++i )
	{
		idata[0] = (stacks - 2) * slices; // top
		idata[1] = i;
		idata[2] = (i + 1) % slices;

		idata[3] = (stacks - 2) * slices + 1; // bottom
		idata[4] = (stacks - 3) * slices + (i + 1) % slices;
		idata[5] = (stacks - 3) * slices + i;

		idata += 6;
	}

	memset(adata, 0, 12 * sizeof(DWORD));

	mesh->UnlockAttributeBuffer();
	mesh->UnlockIndexBuffer();
	mesh->UnlockVertexBuffer();

	*out = mesh;
	return D3D_OK;
}

void DXKillAnyRogueObject()
{
	if( gdiplustoken )
		Gdiplus::GdiplusShutdown(gdiplustoken);
}
