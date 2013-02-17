//=============================================================================================================
#include "pathtracer.h"

#include <iostream>
#include <ctime>
#include <mmsystem.h>

void Worker::Run()
{
	UINT samplesdone;
	RECT desc;

	while( true )
	{
		samplesdone = 0;
		desc = wq.pop();

		if( desc.left == desc.right )
			break;

		while( samplesdone < tracer->NumSamples )
		{
			tracer->RenderSample(desc);

			for( LONG i = desc.top; i < desc.bottom; ++i )
			{
				for( LONG j = desc.left; j < desc.right; ++j )
				{
					D3DXVECTOR4& acc = tracer->accum[i * tracer->Width + j];
					const D3DXVECTOR4& src = tracer->sample[i * tracer->Width + j];

					acc.x += src.x;
					acc.y += src.y;
					acc.z += src.z;
				}
			}

			if( (samplesdone + 1) % 50 == 0 )
				tracer->Visualize(samplesdone, desc);

			++samplesdone;
		}

		tracer->Visualize(tracer->NumSamples, desc);
	}

	Done.Fire();
}

PathTracer::PathTracer()
{
	srand((unsigned int)time(NULL));
	objects.reserve(50);

	samplesdone = 0;
	present = 0;

	NumSamples = 200;
	Width = Height = 0;
}
//=============================================================================================================
PathTracer::~PathTracer()
{
	for( objectlist::iterator it = objects.begin(); it != objects.end(); ++it )
	{
		if( *it )
			delete (*it);
	}
	
	objects.clear();
}
//=============================================================================================================
D3DXVECTOR4 PathTracer::Trace(const Ray& ray, Object* from, int depth)
{
	D3DXVECTOR4		ret(0, 0, 0, 1);
	D3DXVECTOR3		a, b, v, p, n;
	Object*			closest = 0;
	Object*			obj;
	float			dist, mindist = FLT_MAX;

	// find closest object
	for( size_t k = 0; k < objects.size(); ++k )
	{
		obj = objects[k];

		if( obj == from )
			continue;

		if( obj->Intersect(a, b, ray) )
		{
			v = a - ray.Origin;
			dist = LENGTH(v);

			if( dist < mindist )
			{
				p = a;
				n = b;

				closest = obj;
				mindist = dist;
			}
		}
	}

	if( closest )
	{
		if( depth > 5 )
			return D3DXVECTOR4(closest->Material.Emittance, 1);

		Ray newray;
		newray.Origin = p;

		float s = 2 * D3DX_PI * ((rand() % 101) / 100.0f);
		float t = 2 * D3DX_PI * ((rand() % 101) / 100.0f);

		a.x = sinf(s) * cosf(t);
		a.y = cosf(s) * cosf(t);
		a.z = sinf(t);

		s = DOT(a, n);

		if( s < 0 )
			newray.Direction = -1 * a;
		else
			newray.Direction = a;

		ret = Trace(newray, closest, depth + 1);

		ret.x *= closest->Material.Color.r;
		ret.y *= closest->Material.Color.g;
		ret.z *= closest->Material.Color.b;

		ret.x += closest->Material.Emittance.x;
		ret.y += closest->Material.Emittance.y;
		ret.z += closest->Material.Emittance.z;
	}

	return ret;
}
//=============================================================================================================
void PathTracer::Visualize(UINT numdone, const RECT& desc)
{
	if( present )
	{
		presentguard.Lock();
		present(accum, numdone, desc);
		presentguard.Unlock();
	}
}
//=============================================================================================================
void PathTracer::RenderSample(const RECT& rc)
{
	D3DXVECTOR4		tmp, pos;
	D3DXVECTOR4		color;
	Ray				ray;
	float			tmpf;

	for( LONG i = rc.top; i < rc.bottom; ++i )
	{
		for( LONG j = rc.left; j < rc.right; ++j )
		{
			tmp.y = (((float)i / (float)Height) - 0.5f) * -2.0f;
			tmp.x = (((float)j / (float)Width) - 0.5f) * 2.0f;
			tmp.z = 0;
			tmp.w = 1;

			D3DXVec4Transform(&pos, &tmp, &invvp);
			pos /= pos.w;

			ray.Origin = D3DXVECTOR3(invview.m[3][0], invview.m[3][1], invview.m[3][2]);

			ray.Direction.x = pos.x - ray.Origin.x;
			ray.Direction.y = pos.y - ray.Origin.y;
			ray.Direction.z = pos.z - ray.Origin.z;

			NORMALIZE(ray.Direction, ray.Direction);
			
			color = Trace(ray, 0, 1);
			sample[i * Width + j] = color;
		}
	}
}
//=============================================================================================================
void PathTracer::Render(const D3DXMATRIX& view, const D3DXMATRIX& proj)
{
	D3DXMatrixInverse(&invview, NULL, &view);
	D3DXMatrixMultiply(&vp, &view, &proj);
	D3DXMatrixInverse(&invvp, NULL, &vp);

	sample = new D3DXVECTOR4[Width * Height];
	memset(sample, 0, Width * Height * sizeof(D3DXVECTOR4));

	accum = new D3DXVECTOR4[Width * Height];
	memset(accum, 0, Width * Height * sizeof(D3DXVECTOR4));

	workers = (Worker*)malloc(NUM_WORKERS * sizeof(Worker));
	threads = new Thread[NUM_WORKERS];

	for( int i = 0; i < NUM_WORKERS; ++i )
	{
		new (workers + i) Worker(this);
		threads[i].Attach<Worker>(workers + i, &Worker::Run);
	}

	finished = 0;

#if NUM_WORKERS == 0
	RECT rc = { 0, 0, Width, Height };

	while( samplesdone < NumSamples )
	{
		RenderSample(rc);

		for( LONG i = rc.top; i < rc.bottom; ++i )
		{
			for( LONG j = rc.left; j < rc.right; ++j )
			{
				D3DXVECTOR4& acc = accum[i * Width + j];
				D3DXVECTOR4& src = sample[i * Width + j];

				acc.x += src.x;
				acc.y += src.y;
				acc.z += src.z;
			}
		}

		if( present )
			present(accum, samplesdone, rc);

		++samplesdone;
		std::cout << "Samples done: " << samplesdone << "    \r";
	}
#else
	UINT tilesx		= Width / TILE_SIZE;
	UINT tilesy		= Height / TILE_SIZE;
	UINT remx		= Width % TILE_SIZE;
	UINT remy		= Height % TILE_SIZE;
	UINT current	= 0;
	RECT rc;

	for( int i = 0; i < NUM_WORKERS; ++i )
	{
		workcombo.Add(&workers[i].Done);
		threads[i].Start();
	}

	for( UINT i = 0; i < tilesy; ++i )
	{
		for( UINT j = 0; j < tilesx; ++j )
		{
			rc.left		= j * TILE_SIZE;
			rc.right	= rc.left + TILE_SIZE;
			rc.top		= i * TILE_SIZE;
			rc.bottom	= rc.top + TILE_SIZE;

			workers[current].Add(rc);
			current = (current + 1) % NUM_WORKERS;
		}
	}

	rc.left = rc.right = 0;

	for( int i = 0; i < NUM_WORKERS; ++i )
		workers[i].Add(rc);

	workcombo.WaitAll();
#endif

	Kill();
	finished = 1;
}
//=============================================================================================================
void PathTracer::Kill()
{
	if( finished == 0 )
	{
		for( int i = 0; i < NUM_WORKERS; ++i )
		{
			threads[i].Stop();
			threads[i].Close();

			(workers + i)->~Worker();
		}

		delete[] threads;
		free(workers);

		delete[] sample;
		delete[] accum;
	}
}
//=============================================================================================================
