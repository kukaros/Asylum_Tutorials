//=============================================================================================================
#ifndef _PathTracer_H_
#define _PathTracer_H_

#include <vector>
#include <set>

#include "objects.h"
#include "blockingqueue.hpp"

#define NUM_WORKERS		4
#define TILE_SIZE		64

class PathTracer;

class Worker
{
	typedef blockingqueue<RECT> workqueue;

private:
	workqueue wq;
	PathTracer* tracer;

public:
	Signal Done;

	Worker(PathTracer* parent) {
		tracer = parent;
	}

	void Run();

	inline void Add(const RECT& rc) {
		wq.push(rc);
	}
};

class PathTracer
{
	friend class Worker;

private:
	typedef std::vector<Object*> objectlist;

	objectlist		objects;
	D3DXMATRIX		invview;
	D3DXMATRIX		vp, invvp;
	D3DXVECTOR4*	sample;
	D3DXVECTOR4*	accum;
	UINT			samplesdone;
	Guard			presentguard;
	Worker*			workers;
	Thread*			threads;
	SignalCombo		workcombo;
	ATOM			finished;

	void RenderSample(const RECT& rc);
	void Visualize(UINT numdone, const RECT& rc);
	D3DXVECTOR4 Trace(const Ray& ray, Object* from, int depth);

public:
	UINT Width;
	UINT Height;
	UINT NumSamples;

	PathTracer();
	~PathTracer();

	void Kill();
	void Render(const D3DXMATRIX& view, const D3DXMATRIX& proj);
	
	inline void AddObject(Object* obj) {
		objects.push_back(obj);
	}

	inline bool Finished() const {
		return (finished == 1);
	}

	// render callback
	void (*present)(D3DXVECTOR4*, UINT, const RECT&);
};

#endif
//=============================================================================================================
