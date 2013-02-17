//=============================================================================================================
#ifndef _PARTICLEMANAGER_H_
#define _PARTICLEMANAGER_H_

#include <d3dx9.h>
#include <vector>

#include "orderedmultiarray.hpp"

struct BillboardVertex
{
	float x, y, z;
	DWORD color;
	float u, v;
};

struct Particle
{
	D3DXVECTOR3 Position;
	D3DXVECTOR3 Velocity;
	D3DXCOLOR Color;
	unsigned int Life;
};

class ParticleSystem
{
	struct particle_compare
	{
		D3DXMATRIX transform;

		bool operator ()(const Particle& a, const Particle& b) const
		{
			D3DXVECTOR3 p1, p2;

			D3DXVec3TransformCoord(&p1, &a.Position, &transform);
			D3DXVec3TransformCoord(&p2, &b.Position, &transform);

			return (p1.z > p2.z);
		}
	};

	typedef std::vector<Particle> particlelist;
	typedef orderedmultiarray<Particle, particle_compare> orderedparticlelist;

private:
	LPDIRECT3DDEVICE9 d3ddevice;
	LPDIRECT3DVERTEXBUFFER9 vertexbuffer;

	orderedparticlelist ordered;
	particlelist particles;
	size_t maxcount;

public:
	LPDIRECT3DTEXTURE9 ParticleTexture;

	ParticleSystem();
	~ParticleSystem();

	bool Initialize(LPDIRECT3DDEVICE9 device, size_t maxnumparticles);

	void Update();
	void Draw(const D3DXMATRIX& world, const D3DXMATRIX& view);
};

#endif
//=============================================================================================================
