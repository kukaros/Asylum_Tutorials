
#include "drawingitem.h"
#include "renderingcore.h"

// *****************************************************************************************************************************
//
// NativeContext impl
//
// *****************************************************************************************************************************

class NativeContext::FlushPrimitivesTask : public RenderingCore::IRenderingTask
{
	typedef std::vector<Point4> _vertexlist;
	typedef std::vector<GLushort> _indexlist;

private:
	float				world[16];
	_vertexlist			vertices;
	_indexlist			indices;
	OpenGLFramebuffer*	rendertarget;	// external
	OpenGLMesh*			mesh;
	OpenGLEffect*		lineeffect;
	OpenGLColor			clearcolor;
	OpenGLColor			linecolor;
	bool				needsclear;

	void Dispose()
	{
		// NOTE: runs on renderer thread
		SAFE_DELETE(lineeffect);
		SAFE_DELETE(mesh);
	}

	void Execute(IRenderingContext* context)
	{
		// NOTE: runs on renderer thread
		OpenGLVertexElement decl[] =
		{
			{ 0, 0, GLDECLTYPE_FLOAT4, GLDECLUSAGE_POSITION, 0 },
			{ 0xff, 0, 0, 0, 0 }
		};

		OpenGLAttributeRange table[] =
		{
			{ GLPT_LINELIST, 0, 0, indices.size(), 0, vertices.size() }
		};

		float	width			= (float)rendertarget->GetWidth();
		float	height			= (float)rendertarget->GetHeight();
		float	proj[16];
		float	thickness[2]	= { 3.0f / width, 3.0f / height };
		void*	vdata			= 0;
		void*	idata			= 0;

		if( !lineeffect )
		{
			lineeffect = context->CreateEffect(
				"../media/shadersGL/color.vert",
				"../media/shadersGL/renderlines.geom",
				"../media/shadersGL/color.frag");
		}

		if( !mesh )
			mesh = context->CreateMesh(128, 256, GLMESH_DYNAMIC, decl);

		mesh->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
		mesh->LockIndexBuffer(0, 0, GLLOCK_DISCARD, &idata);

		memcpy(vdata, vertices.data(), vertices.size() * sizeof(Point4));
		memcpy(idata, indices.data(), indices.size() * sizeof(GLushort));

		mesh->UnlockIndexBuffer();
		mesh->UnlockVertexBuffer();
		mesh->SetAttributeTable(table, 1);

		GLMatrixOrthoRH(proj, width * -0.5f, width * 0.5f, height * -0.5f, height * 0.5f, -1, 1);

		lineeffect->SetMatrix("matWorld", world);
		lineeffect->SetMatrix("matViewProj", proj);
		lineeffect->SetVector("color", &linecolor.r);
		lineeffect->SetVector("lineThickness", thickness);

		rendertarget->Set();
		{
			if( needsclear )
				context->Clear(clearcolor);

			lineeffect->Begin();
			{
				mesh->DrawSubset(0);
			}
			lineeffect->End();
		}
		rendertarget->Unset();

		needsclear = false;
	}

public:
	typedef _vertexlist vertexlist;
	typedef _indexlist indexlist;

	FlushPrimitivesTask(int universe, OpenGLFramebuffer* framebuffer)
		: IRenderingTask(universe)
	{
		// NOTE: runs on any other thread
		rendertarget	= framebuffer;
		lineeffect		= 0;
		mesh			= 0;

		clearcolor		= OpenGLColor(1, 1, 1, 1);
		linecolor		= OpenGLColor(1, 1, 1, 1);
		needsclear		= false;

		GLMatrixIdentity(world);
	}

	void SetNeedsClear(const OpenGLColor& color)
	{
		// NOTE: runs on any other thread
		needsclear = true;
		clearcolor = color;
	}

	void SetData(const vertexlist& verts, const indexlist& inds)
	{
		// NOTE: runs on any other thread
		vertices = verts;
		indices = inds;
	}

	void SetWorldMatrix(float matrix[16])
	{
		memcpy(world, matrix, 16 * sizeof(float));
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

	vertices.reserve(128);
	indices.reserve(256);
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
	flushtask->SetData(vertices, indices);

	GetRenderingCore()->AddTask(flushtask);
	flushtask->Wait(); //

	vertices.clear();
	indices.clear();
}

void NativeContext::Clear(const OpenGLColor& color)
{
	if( flushtask )
		flushtask->SetNeedsClear(color);
}

void NativeContext::MoveTo(float x, float y)
{
	if( (indices.size() > 0 && vertices.size() + 1 > indices.size()) ||
		(indices.size() == 0 && vertices.size() == 1) )
	{
		// detected 2 MoveTo()s in a row
		vertices.pop_back();
	}

	vertices.push_back(Point4(x, y, 0, 1));
}

void NativeContext::LineTo(float x, float y)
{
	if( vertices.size() == 0 )
		vertices.push_back(Point4(0, 0, 0, 1));

	indices.push_back((unsigned short)vertices.size() - 1);
	indices.push_back((unsigned short)vertices.size());

	vertices.push_back(Point4(x, y, 0, 1));
}

void NativeContext::SetWorldTransform(float transform[16])
{
	if( flushtask )
		flushtask->SetWorldMatrix(transform);
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

		context->Present(universeid);
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
