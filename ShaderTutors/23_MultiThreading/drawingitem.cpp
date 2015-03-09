
#include "drawingitem.h"
#include "renderingcore.h"

// *****************************************************************************************************************************
//
// NativeContext impl
//
// *****************************************************************************************************************************

class NativeContext::FlushPrimitivesTask : public RenderingCore::IRenderingTask
{
private:
	OpenGLFramebuffer*	rendertarget;
	OpenGLColor			clearcolor;
	OpenGLEffect*		lineeffect;
	bool				needsclear;

	void Dispose()
	{
		// NOTE: runs on renderer thread
		SAFE_DELETE(lineeffect);
	}

	void Execute(IRenderingContext* context)
	{
		// NOTE: runs on renderer thread
		if( !lineeffect )
		{
			lineeffect = context->CreateEffect(
				"../media/shadersGL/color.vert",
				"../media/shadersGL/renderlines.geom",
				"../media/shadersGL/color.frag");
		}

		rendertarget->Set();
		{
			if( needsclear )
				context->Clear(clearcolor);
		}
		rendertarget->Unset();

		needsclear = false;
	}

public:
	FlushPrimitivesTask(int universe, OpenGLFramebuffer* framebuffer)
		: IRenderingTask(universe)
	{
		// NOTE: runs on any other thread
		rendertarget	= framebuffer;
		lineeffect		= 0;
		clearcolor		= OpenGLColor(1, 1, 1, 1);
		needsclear		= false;
	}

	void SetNeedsClear(const OpenGLColor& color)
	{
		// NOTE: runs on any other thread
		needsclear = true;
		clearcolor = color;
	}
};

NativeContext::NativeContext()
{
	owneritem = 0;
	ownerlayer = 0;
	flushtask = 0;
}

NativeContext::NativeContext(const NativeContext& other)
{
	owneritem = 0;
	ownerlayer = 0;
	flushtask = 0;

	operator =(other);
}

NativeContext::NativeContext(DrawingItem* item, DrawingLayer* layer)
{
	owneritem = item;
	ownerlayer = layer;
	flushtask = new FlushPrimitivesTask(item->GetOpenGLUniverseID(), layer->GetRenderTarget());
}

NativeContext::~NativeContext()
{
	if( flushtask )
	{
		FlushPrimitives();

		flushtask->Release();
		flushtask = 0;
	}

	if( ownerlayer )
		ownerlayer->contextguard.Unlock();
}

void NativeContext::FlushPrimitives()
{
	GetRenderingCore()->AddTask(flushtask);
}

void NativeContext::Clear(const OpenGLColor& color)
{
	if( flushtask )
		flushtask->SetNeedsClear(color);
}

void NativeContext::MoveTo(float x, float y)
{
}

void NativeContext::LineTo(float x, float y)
{
}

NativeContext& NativeContext::operator =(const NativeContext& other)
{
	// this is a forced move semantic
	if( &other == this )
		return *this;

	if( flushtask )
		flushtask->Release();

	flushtask = other.flushtask;
	owneritem = other.owneritem;
	ownerlayer = other.ownerlayer;

	other.flushtask = 0;
	other.owneritem = 0;
	other.ownerlayer = 0;

	return *this;
}

// *****************************************************************************************************************************
//
// DrawingLayer impl
//
// *****************************************************************************************************************************

class DrawingLayer::DrawingLayerSetupTask : public RenderingCore::IRenderingTask
{
private:
	OpenGLFramebuffer*	rendertarget;
	GLuint				rtwidth, rtheight;

	void Dispose()
	{
		// NOTE: runs on renderer thread
		SAFE_DELETE(rendertarget);
	}

	void Execute(IRenderingContext* context)
	{
		// NOTE: runs on renderer thread
		if( !rendertarget )
		{
			rendertarget = context->CreateFramebuffer(rtwidth, rtheight);

			rendertarget->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A8R8G8B8, GL_LINEAR);
			rendertarget->AttachRenderbuffer(GL_DEPTH_STENCIL_ATTACHMENT, GLFMT_D24S8);
		}
	}

public:
	DrawingLayerSetupTask(int universe, GLuint width, GLuint height)
		: IRenderingTask(universe)
	{
		// NOTE: runs on any other thread
		rendertarget	= 0;
		rtwidth			= width;
		rtheight		= height;
	}

	void Setup()
	{
		// NOTE: runs on any other thread
		GetRenderingCore()->AddTask(this);
		Wait();
	}

	inline OpenGLFramebuffer* GetRenderTarget() const {
		return rendertarget;
	}
};

DrawingLayer::DrawingLayer(int universe, unsigned int width, unsigned int height)
{
	owner = 0;

	setuptask = new DrawingLayerSetupTask(universe, width, height);
	setuptask->Setup();
}

DrawingLayer::~DrawingLayer()
{
	setuptask->Release();
	setuptask = 0;
}

OpenGLFramebuffer* DrawingLayer::GetRenderTarget() const
{
	return setuptask->GetRenderTarget();
}

NativeContext DrawingLayer::GetContext()
{
	contextguard.Lock();
	return NativeContext(owner, this);
}

// *****************************************************************************************************************************
//
// DrawingItem impl
//
// *****************************************************************************************************************************

class DrawingItem::RecomposeLayersTask : public RenderingCore::IRenderingTask
{
private:
	DrawingItem*		item;
	OpenGLScreenQuad*	screenquad;
	OpenGLEffect*		effect;

	void Dispose()
	{
		// NOTE: runs on renderer thread
		SAFE_DELETE(screenquad);
		SAFE_DELETE(effect);
	}

	void Execute(IRenderingContext* context)
	{
		// NOTE: runs on renderer thread
		if( !item )
			return;

		if( !screenquad )
			screenquad = context->CreateScreenQuad();

		if( !effect )
			effect = context->CreateEffect("../media/shadersGL/basic2D.vert", 0, "../media/shadersGL/basic2D.frag");

		if( effect && screenquad )
		{
			float texmat[16];
			GLuint texid = item->GetBottomLayer().GetRenderTarget()->GetColorAttachment(0);
				
			GLMatrixIdentity(texmat);

			glBindTexture(GL_TEXTURE_2D, texid); // TODO: vedeni
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT);

			effect->SetInt("sampler0", 0);
			effect->SetMatrix("matTexture", texmat);
			effect->Begin();
			{
				screenquad->Draw();

				// TODO: feedback
			}
			effect->End();
		}
	}

public:
	RecomposeLayersTask(int universe, DrawingItem* drawingitem)
		: IRenderingTask(universe)
	{
		// NOTE: runs on any other thread
		screenquad	= 0;
		effect		= 0;
		item		= drawingitem;
	}

	void Render()
	{
		// NOTE: runs on any other thread
		GetRenderingCore()->AddTask(this);
		Wait();
	}
};

DrawingItem::DrawingItem(int universe, unsigned int width, unsigned int height)
	: bottomlayer(universe, width, height), feedbacklayer(universe, width, height)
{
	universeid = universe;
	recomposetask = new RecomposeLayersTask(universeid, this);

	// DON'T USE 'this' in initializer list!!!
	bottomlayer.owner = this;
	feedbacklayer.owner = this;
}

DrawingItem::~DrawingItem()
{
	recomposetask->Release();
	recomposetask = 0;
}

void DrawingItem::RecomposeLayers()
{
	recomposetask->Render();
}
