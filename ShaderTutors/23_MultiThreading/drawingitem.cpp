
#include "drawingitem.h"
#include "renderingcore.h"

NativeContext::NativeContext()
{
	owneritem = 0;
	ownerlayer = 0;
}

NativeContext::NativeContext(DrawingItem* item, const DrawingLayer* layer)
{
	owneritem = item;
	ownerlayer = layer;
}

void NativeContext::MoveTo(float x, float y)
{
}

void NativeContext::LineTo(float x, float y)
{
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
	bool				disposing;

	void Execute(IRenderingContext* context)
	{
		// NOTE: runs on renderer thread
		if( disposing )
		{
			SAFE_DELETE(rendertarget);
		}
		else
		{
			if( !rendertarget )
			{
				rendertarget = context->CreateFramebuffer(rtwidth, rtheight);

				rendertarget->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A8R8G8B8, GL_LINEAR);
				rendertarget->AttachRenderbuffer(GL_DEPTH_STENCIL_ATTACHMENT, GLFMT_D24S8);
			}
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
		disposing		= false;
	}

	void Dispose()
	{
		// NOTE: runs on any other thread
		disposing = true;

		GetRenderingCore()->AddTask(this);
		Wait();
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
	setuptask->Dispose();

	delete setuptask;
	setuptask = 0;
}

OpenGLFramebuffer* DrawingLayer::GetRenderTarget() const
{
	return setuptask->GetRenderTarget();
}

NativeContext DrawingLayer::GetContext() const
{
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
	bool				disposing;

	void Execute(IRenderingContext* context)
	{
		// NOTE: runs on renderer thread
		if( disposing )
		{
			SAFE_DELETE(screenquad);
			SAFE_DELETE(effect);
		}
		else if( item )
		{
			if( !screenquad )
				screenquad = context->CreateScreenQuad();

			if( !effect )
				effect = context->CreateEffect("../media/shadersGL/basic2D.vert", 0, "../media/shadersGL/basic2D.frag");

			if( effect && screenquad )
			{
				float texmat[16];
				GLMatrixIdentity(texmat);

				GLuint texid = item->GetBottomLayer().GetRenderTarget()->GetColorAttachment(0);
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
	}

public:
	RecomposeLayersTask(int universe, DrawingItem* drawingitem)
		: IRenderingTask(universe)
	{
		// NOTE: runs on any other thread
		screenquad	= 0;
		effect		= 0;
		item		= drawingitem;
		disposing	= false;
	}

	void Dispose()
	{
		// NOTE: runs on any other thread
		disposing = true;

		GetRenderingCore()->AddTask(this);
		Wait();
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
	recomposetask->Dispose();

	delete recomposetask;
	recomposetask = 0;
}

void DrawingItem::RecomposeLayers()
{
	recomposetask->Render();
}
