
#ifndef _DRAWINGITEM_H_
#define _DRAWINGITEM_H_

#include "../common/thread.h"

class DrawingItem;
class DrawingLayer;
class OpenGLFramebuffer;
class OpenGLScreenQuad;
class OpenGLColor;

class NativeContext
{
	friend class DrawingLayer;
	class FlushPrimitivesTask;

private:
	mutable DrawingItem*			owneritem;
	mutable DrawingLayer*			ownerlayer;
	mutable FlushPrimitivesTask*	flushtask;

	NativeContext();
	NativeContext(DrawingItem* item, DrawingLayer* layer);

	void FlushPrimitives();

public:
	NativeContext(const NativeContext& other);
	~NativeContext();

	void Clear(const OpenGLColor& color);
	void MoveTo(float x, float y);
	void LineTo(float x, float y);

	NativeContext& operator =(const NativeContext& other);
};

class DrawingLayer
{
	friend class DrawingItem;
	friend class NativeContext;

	class DrawingLayerSetupTask;

private:
	DrawingItem*			owner;
	DrawingLayerSetupTask*	setuptask;
	Guard					contextguard;

	DrawingLayer(int universe, unsigned int width, unsigned int height);
	~DrawingLayer();

public:
	NativeContext GetContext();
	OpenGLFramebuffer* GetRenderTarget() const;
};

class DrawingItem
{
	class RecomposeLayersTask;

private:
	DrawingLayer			bottomlayer;
	DrawingLayer			feedbacklayer;
	RecomposeLayersTask*	recomposetask;
	int						universeid;

public:
	DrawingItem(int universe, unsigned int width, unsigned int height);
	~DrawingItem();

	void RecomposeLayers();

	inline DrawingLayer& GetBottomLayer() {
		return bottomlayer;
	}

	inline DrawingLayer& GetFeedbackLayer() {
		return feedbacklayer;
	}

	inline int GetOpenGLUniverseID() const {
		return universeid;
	}
};

#endif





//#include "../common/glext.h"
//#include <vector>

/*
class RenderingCore;

class NativeContext
{
	class FlushPrimitivesTask;
	typedef std::vector<FlushPrimitivesTask*> rendertasklist;

	RenderingCore*			renderingcore;
	int						universeid;
	FlushPrimitivesTask*	flusher;
	OpenGLFramebuffer*		layer;
	rendertasklist			tasks;

public:
	NativeContext(RenderingCore* core, int universe);
	~NativeContext();

	void FlushPrimitives();
};
*/