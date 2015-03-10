
#include "win32window.h"
#include "renderingcore.h"
#include "drawingitem.h"

// *****************************************************************************************************************************
//
// Window 1 functions
//
// *****************************************************************************************************************************

void Window1_Created(Win32Window* window)
{
}

void Window1_Closing(Win32Window*)
{
}

void Window1_Render(Win32Window* window, float alpha, float elapsedtime)
{
	static float time = 0;
	time += elapsedtime;

	DrawingItem* drawingitem = window->GetDrawingItem();
	DrawingLayer& bottomlayer = drawingitem->GetBottomLayer();
	{
		NativeContext	context		= bottomlayer.GetContext();
		float			world[16];
		float			bigradius	= 150;
		float			smallradius	= 80;
		float			m2pi		= 6.293185f;
		int				segments	= 16;

		GLMatrixRotationAxis(world, fmodf(time * 20.0f, 360.0f) * (3.14152f / 180.0f), 0, 0, 1);

		context.Clear(OpenGLColor(0.0f, 0.125f, 0.3f, 1.0f));
		context.SetWorldTransform(world);
		context.MoveTo(0, bigradius);

		for( int i = 1; i <= segments; ++i )
		{
			if( i % 2 == 1 )
			{
				context.LineTo(
					sinf((m2pi / segments) * i) * smallradius,
					cosf((m2pi / segments) * i) * smallradius);
			}
			else
			{
				context.LineTo(
					sinf((m2pi / segments) * i) * bigradius,
					cosf((m2pi / segments) * i) * bigradius);
			}
		}
	}

	drawingitem->RecomposeLayers();
}
