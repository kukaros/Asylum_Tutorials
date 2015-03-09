
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

	DrawingItem* drawingitem = window->GetDrawingItem();
	time += elapsedtime;

	DrawingLayer& bottomlayer = drawingitem->GetBottomLayer();
	{
		NativeContext context = bottomlayer.GetContext();

		context.Clear(OpenGLColor(0.0f, 0.125f, 0.3f, 1.0f));
		context.MoveTo(0, 0);
		context.LineTo(1, 1);
	}

	drawingitem->RecomposeLayers();
	window->Present();
}
