//*************************************************************************************************************
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "GdiPlus.lib")

#include <Windows.h>
#include <GdiPlus.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>

#include "qgl2extensions.h"

// helper macros
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

// external variables
extern HDC hdc;
extern long screenwidth;
extern long screenheight;

extern void DrawPlanes();
extern void DrawCube();

// tutorial variables
GLuint msaafbo		= 0;
GLuint msaarb		= 0;
GLuint msaadepth	= 0;
GLuint resolvefbo	= 0;
GLuint resolvetex1	= 0;
GLuint tex1			= 0;
GLuint tex2			= 0;

bool bugmode = false;
ULONG_PTR gdiplustoken;

void PreRenderText(const std::string& str, GLuint texid, GLsizei width, GLsizei height)
{
	if( texid == 0 )
		return;

	Gdiplus::Color				outline(0xff000000);
	Gdiplus::Color				fill(0xffffffff);

	Gdiplus::Bitmap*			bitmap;
	Gdiplus::Graphics*			graphics;
	Gdiplus::GraphicsPath		path;
	Gdiplus::FontFamily			family(L"Arial");
	Gdiplus::StringFormat		format;
	Gdiplus::Pen				pen(outline, 3);
	Gdiplus::SolidBrush			brush(fill);
	std::wstring				wstr(str.begin(), str.end());

	bitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
	graphics = new Gdiplus::Graphics(bitmap);

	// render text
	graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	graphics->SetPageUnit(Gdiplus::UnitPixel);

	path.AddString(wstr.c_str(), wstr.length(), &family, Gdiplus::FontStyleBold, 25, Gdiplus::Point(0, 0), &format);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	graphics->DrawPath(&pen, &path);
	graphics->FillPath(&brush, &path);

	// copy to texture
	Gdiplus::Rect rc(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	Gdiplus::BitmapData data;

	memset(&data, 0, sizeof(Gdiplus::BitmapData));
	bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);

	glBindTexture(GL_TEXTURE_2D, texid);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.Scan0);
	glBindTexture(GL_TEXTURE_2D, 0);

	bitmap->UnlockBits(&data);

	delete graphics;
	delete bitmap;
}

bool InitScene()
{
	Gdiplus::GdiplusStartupInput gdiplustartup;
	Gdiplus::GdiplusStartup(&gdiplustoken, &gdiplustartup, NULL);

	Quadron::qGL2Extensions::QueryFeatures();

	// setup opengl
	glClearColor(1, 1, 1, 1);
	//glClearColor(0.4f, 0.58f, 0.93f, 1.0f);
	glClearDepth(1);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glShadeModel(GL_SMOOTH);

	glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0f);
	glLightModeli(0x81F8, 0x81FA);

	float spec[] = { 1, 1, 1, 1 };

	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);

	// setup MSAA FBO
	glGenFramebuffers(1, &msaafbo);
	glBindFramebuffer(GL_FRAMEBUFFER, msaafbo);

	glGenRenderbuffers(1, &msaarb);
	glBindRenderbuffer(GL_RENDERBUFFER, msaarb);

	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA8, screenwidth, screenheight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaarb);

	glGenRenderbuffers(1, &msaadepth);
	glBindRenderbuffer(GL_RENDERBUFFER, msaadepth);

	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT24, screenwidth, screenheight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msaadepth);

	if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
		MYERROR("InitScene(): MSAA framebuffer is not complete");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// setup resolve target
	glGenFramebuffers(1, &resolvefbo);
	glBindFramebuffer(GL_FRAMEBUFFER, resolvefbo);

	glGenTextures(1, &resolvetex1);
	glBindTexture(GL_TEXTURE_2D, resolvetex1);

	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA8, screenwidth, screenheight,
		0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolvetex1, 0);

	if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
		MYERROR("InitScene(): Resolve framebuffer is not complete");

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// pre-render text
	glGenTextures(1, &tex1);
	glBindTexture(GL_TEXTURE_2D, tex1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	PreRenderText("Press Space to reproduce bug", tex1, 600, 60);

	glGenTextures(1, &tex2);
	glBindTexture(GL_TEXTURE_2D, tex2);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	PreRenderText("Calling glBlitFramebuffer 2 times,\nfirst call overwrites MSAA buffer", tex2, 600, 60);

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	if( tex1 != 0 )
		glDeleteTextures(1, &tex1);

	if( tex2 != 0 )
		glDeleteTextures(1, &tex2);

	if( msaarb != 0 )
		glDeleteRenderbuffers(1, &msaarb);

	if( msaadepth != 0 )
		glDeleteRenderbuffers(1, &msaadepth);

	if( resolvetex1 != 0 )
		glDeleteTextures(1, &resolvetex1);

	if( msaafbo != 0 )
		glDeleteFramebuffers(1, &msaafbo);

	if( resolvefbo != 0 )
		glDeleteFramebuffers(1, &resolvefbo);

	tex1 = 0;

	Gdiplus::GdiplusShutdown(gdiplustoken);
}
//*************************************************************************************************************
void Update(float delta)
{
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	if( wparam == VK_SPACE || wparam == VK_RETURN )
		bugmode = !bugmode;
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, msaafbo);

	// planes
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (double)screenwidth / (double)screenheight, 2.1, 6);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, -5, 0, 0, 0, 0, 0, 0, 1);
	glRotatef(-6, 0, 1, 0);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	DrawPlanes();

	// cube
	float angle = time * 90.0f / 3.14152f;
	float lightpos[] = { 0, -1, 0, 0 };

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, -5, 0, 0, 0, 0, 0, 0, 1);

	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	glRotatef(angle, 1, 0, 0);
	glRotatef(angle, 0, 1, 0);
	glRotatef(angle, 0, 0, 1);

	DrawCube();

	glDisable(GL_LIGHTING);

	// blit
	glBindFramebuffer(GL_READ_FRAMEBUFFER, msaafbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolvefbo);

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glBlitFramebuffer(
		0, 0, screenwidth, screenheight,
		0, 0, screenwidth, screenheight,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);

	if( bugmode )
	{
		glBlitFramebuffer(
			0, 0, screenwidth, screenheight,
			0, 0, screenwidth, screenheight,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glReadBuffer(GL_BACK);
	glDrawBuffer(GL_BACK);

	// render to backbuffer
	glDisable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, resolvetex1);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glBegin(GL_QUADS);
	{
		glTexCoord2f(0, 0);
		glVertex3f(-1, -1, 0);

		glTexCoord2f(1, 0);
		glVertex3f(1, -1, 0);

		glTexCoord2f(1, 1);
		glVertex3f(1, 1, 0);

		glTexCoord2f(0, 1);
		glVertex3f(-1, 1, 0);
	}
	glEnd();

	// text
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if( bugmode )
		glBindTexture(GL_TEXTURE_2D, tex2);
	else
		glBindTexture(GL_TEXTURE_2D, tex1);

	float rc_ws[] =
	{
		10,
		screenheight - 10.0f,
		510,
		screenheight - 70.0f
	};

	float rc_ss[] =
	{
		2.0f * rc_ws[0] / screenwidth - 1,
		2.0f * rc_ws[1] / screenheight - 1,
		2.0f * rc_ws[2] / screenwidth - 1,
		2.0f * rc_ws[3] / screenheight - 1
	};

	glBegin(GL_QUADS);
	{
		glTexCoord2f(0, 1);
		glVertex3f(rc_ss[0], rc_ss[3], 0);

		glTexCoord2f(1, 1);
		glVertex3f(rc_ss[2], rc_ss[3], 0);

		glTexCoord2f(1, 0);
		glVertex3f(rc_ss[2], rc_ss[1], 0);

		glTexCoord2f(0, 0);
		glVertex3f(rc_ss[0], rc_ss[1], 0);
	}
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	time += elapsedtime;

	// present
	SwapBuffers(hdc);
}
//*************************************************************************************************************
