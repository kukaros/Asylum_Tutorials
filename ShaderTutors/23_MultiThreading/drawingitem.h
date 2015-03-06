
#ifndef _DRAWINGITEM_H_
#define _DRAWINGITEM_H_

class DrawingItem;
class DrawingLayer;
class OpenGLFramebuffer;
class OpenGLScreenQuad;

class NativeContext
{
	friend class DrawingLayer;

private:
	DrawingItem* owneritem;
	const DrawingLayer* ownerlayer;

	NativeContext(DrawingItem* item, const DrawingLayer* layer);

public:
	NativeContext();

	void MoveTo(float x, float y);
	void LineTo(float x, float y);
};

class DrawingLayer
{
	friend class DrawingItem;
	class DrawingLayerSetupTask;

private:
	DrawingItem*			owner;
	DrawingLayerSetupTask*	setuptask;

	DrawingLayer(int universe, unsigned int width, unsigned int height);
	~DrawingLayer();

public:
	NativeContext GetContext() const;
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

	inline const DrawingLayer& GetBottomLayer() const {
		return bottomlayer;
	}

	inline const DrawingLayer& GetFeedbackLayer() const {
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