
#ifndef _DRAWINGITEM_H_
#define _DRAWINGITEM_H_

#include "../common/thread.h"

class DrawingItem;
class DrawingLayer;
class OpenGLFramebuffer;
class OpenGLScreenQuad;
class OpenGLColor;

struct Point4
{
	float x, y, z, w;

	Point4(float _x, float _y, float _z, float _w)
		: x(_x), y(_y), z(_z), w(_w)
	{
	}
};

/**
 * \brief Interface for 2D rendering
 *
 * Copy constructor and operator = assumes move semantic.
 */
class NativeContext
{
	friend class DrawingLayer;
	class FlushPrimitivesTask;

	typedef std::vector<Point4> vertexlist;
	typedef std::vector<unsigned short> indexlist;

private:
	mutable DrawingItem*			owneritem;
	mutable DrawingLayer*			ownerlayer;
	mutable FlushPrimitivesTask*	flushtask;

	vertexlist	vertices;
	indexlist	indices;

	NativeContext();
	NativeContext(DrawingItem* item, DrawingLayer* layer);

	void FlushPrimitives();

public:
	NativeContext(const NativeContext& other);
	~NativeContext();

	void Clear(const OpenGLColor& color);
	void MoveTo(float x, float y);
	void LineTo(float x, float y);
	void SetWorldTransform(float transform[16]);
	void SetColor(const OpenGLColor& color);

	NativeContext& operator =(const NativeContext& other);
};

/**
 * \brief One layer with attached rendertarget
 */
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

/**
 * \brief Multilayered drawing sheet
 */
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
