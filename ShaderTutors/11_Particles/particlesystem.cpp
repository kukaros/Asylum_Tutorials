//=============================================================================================================
#include "particlesystem.h"
#include <iostream>

ParticleSystem::ParticleSystem()
{
	vertexbuffer = NULL;
	maxcount = 0;

	ParticleTexture = NULL;
}
//=============================================================================================================
ParticleSystem::~ParticleSystem()
{
	if( vertexbuffer )
	{
		vertexbuffer->Release();
		vertexbuffer = NULL;
	}
}
//=============================================================================================================
bool ParticleSystem::Initialize(LPDIRECT3DDEVICE9 device, size_t maxnumparticles)
{
	if( !device )
		return false;

	d3ddevice = device;
	maxcount = maxnumparticles;

	HRESULT hr = d3ddevice->CreateVertexBuffer(maxcount * sizeof(BillboardVertex) * 6,
		D3DUSAGE_DYNAMIC, D3DFVF_XYZ|D3DFVF_TEX1|D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vertexbuffer, NULL);

	if( FAILED(hr) )
		return false;

	particles.reserve(maxnumparticles);
	ordered.reserve(maxnumparticles);

	return true;
}
//=============================================================================================================
void ParticleSystem::Update()
{
	//float fuzzy = (float)(rand() % 100) / 100.0f;
	D3DXVECTOR3 Force(0, 0.001f, 0);
	D3DXCOLOR inner(0xffFFEB89);
	D3DXCOLOR mid(0xffFF9347);
	D3DXCOLOR outer(0xffff2222);

	// részecskék frissitése
	size_t i = 0;

	while( i < particles.size() )
	{
		Particle& p = particles[i];
		
		if( p.Life < 30 )
			p.Color.a = (float)p.Life / 30.0f;

		// ha meghalt, akkor csere és nem lépünk
		if( p.Life == 0 )
		{
			std::swap(particles[i], particles[particles.size() - 1]);
			particles.pop_back();
		}
		else
		{
			p.Position += p.Velocity;
			p.Velocity += Force;

			--p.Life;
			++i;
		}
	}

	// uj részecskék generálása (ha lehet)
	D3DXVECTOR3 EmitterPosition(0, 0, 0);
	float EmitterRadius = 0.5f;

	while( particles.size() < maxcount )
	{
		// keresünk egy random poziciot a gömbön
		float u = (float)(rand() % 100) / 100.0f;
		float v = (float)(rand() % 100) / 100.0f;

		u = 2 * u * D3DX_PI;
		v = (v - 0.5f) * D3DX_PI;

		Particle p;
		D3DXVECTOR3 du, dv, n;

		p.Position.x = EmitterRadius * sin(u) * cos(v);
		p.Position.y = EmitterRadius * cos(u) * cos(v);
		p.Position.z = EmitterRadius * sin(v);

		du.x = p.Position.y;
		du.y = -p.Position.x;
		du.z = p.Position.z;

		dv.x = -EmitterRadius * sin(u) * sin(v);
		dv.y = -EmitterRadius * cos(u) * sin(v);
		dv.z = EmitterRadius * cos(v);

		D3DXVec3Cross(&n, &du, &dv);
		D3DXVec3Normalize(&n, &n);

		p.Velocity = n * 0.02f; // * InitialVelocity
		p.Life = (rand() % 50) + 10; // InitialLife
		p.Color = D3DXCOLOR(1, 1, 1, 1);

		particles.push_back(p);
	}

	//std::cout << particles.size() << "        \r";
}
//=============================================================================================================
void ParticleSystem::Draw(const D3DXMATRIX& world, const D3DXMATRIX& view)
{
	D3DXVECTOR3 wp, tmp1, tmp2, right, up;
	float ParticleSize = 0.5f;
	float hs = ParticleSize * 0.5f;
	BillboardVertex* vdata = NULL;

	HRESULT hr = vertexbuffer->Lock(0, 0, (void**)&vdata, D3DLOCK_DISCARD);

	right.x = view._11;
	right.y = view._21;
	right.z = view._31;

	up.x = view._12;
	up.y = view._22;
	up.z = view._32;

	D3DXMatrixMultiply(&ordered.comp.transform, &world, &view);

	tmp1 = (right - up) * hs;
	tmp2 = (right + up) * hs;
	
	if( SUCCEEDED(hr) )
	{
		ordered.clear();

		for( size_t i = 0; i < particles.size(); ++i )
			ordered.insert(particles[i]);

		for( size_t i = 0; i < ordered.size(); ++i )
		{
			BillboardVertex* v1 = (vdata + i * 6 + 0);
			BillboardVertex* v2 = (vdata + i * 6 + 1);
			BillboardVertex* v3 = (vdata + i * 6 + 2);
			BillboardVertex* v4 = (vdata + i * 6 + 3);
			BillboardVertex* v5 = (vdata + i * 6 + 4);
			BillboardVertex* v6 = (vdata + i * 6 + 5);

			const Particle& p = ordered[i];
			D3DXVec3TransformCoord(&wp, &p.Position, &world);
			
			// topleft
			v1->x = wp.x - tmp1.x;
			v1->y = wp.y - tmp1.y;
			v1->z = wp.z - tmp1.z;

			v1->u = 0;
			v1->v = 0;
			v1->color = p.Color;

			// topright
			v2->x = v5->x = wp.x + tmp2.x;
			v2->y = v5->y = wp.y + tmp2.y;
			v2->z = v5->z = wp.z + tmp2.z;

			v2->u = v5->u = 1;
			v2->v = v5->v = 0;
			v2->color = v5->color = p.Color;

			// bottomleft
			v3->x = v4->x = wp.x - tmp2.x;
			v3->y = v4->y = wp.y - tmp2.y;
			v3->z = v4->z = wp.z - tmp2.z;

			v3->u = v4->u = 0;
			v3->v = v4->v = 1;
			v3->color = v4->color = p.Color;

			// bottomright
			v6->x = wp.x + tmp1.x;
			v6->y = wp.y + tmp1.y;
			v6->z = wp.z + tmp1.z;

			v6->u = 1;
			v6->v = 1;
			v6->color = p.Color;
		}

		vertexbuffer->Unlock();

		d3ddevice->SetStreamSource(0, vertexbuffer, 0, sizeof(BillboardVertex));
		d3ddevice->SetIndices(NULL);

		d3ddevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		d3ddevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		d3ddevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE); // D3DBLEND_INVSRCALPHA
		d3ddevice->SetRenderState(D3DRS_ZENABLE, false);
		d3ddevice->SetTexture(0, ParticleTexture);

		d3ddevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		d3ddevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		d3ddevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

		d3ddevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		d3ddevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		d3ddevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

		d3ddevice->SetFVF(D3DFVF_XYZ|D3DFVF_TEX1|D3DFVF_DIFFUSE);

		d3ddevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, particles.size() * 2);
		d3ddevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
		d3ddevice->SetRenderState(D3DRS_ZENABLE, true);
	}
}
//=============================================================================================================
