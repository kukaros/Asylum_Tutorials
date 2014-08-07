//*************************************************************************************************************
#include <Windows.h>
#include <iostream>

#include "../common/glext.h"

// TODO:
// - surfacet megjeleniteni
// - kiböviteni az openglmesh-t es inkabb azt hasznalni
// - alulra valami
// - MSAA

// helper macros
#define TITLE				"Shader sample 22.3: Tessellating NURBS surfaces"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_DELETE(x)		if( (x) ) { delete (x); (x) = 0; }

#define M_PI				3.141592f
#define NUM_SEGMENTS		50

// external variables
extern HWND		hwnd;
extern HDC		hdc;
extern long		screenwidth;
extern long		screenheight;
extern short	mousex, mousedx;
extern short	mousey, mousedy;
extern short	mousedown;

// sample variables
float controlpoints[][3] =
{
	{ 1, 1, 0 },
	{ 1, 5, 0 },
	{ 3, 6, 0 },
	{ 6, 3, 0 },
	{ 9, 4, 0 },
	{ 9, 9, 0 },
	{ 5, 6, 0 }
};

float weights[] =
{
	1, 1, 1, 1, 1, 1, 1
};

float knot[] =
{
	0, 0, 0, 0, 0.4f, 0.4f, 0.4f, 1, 1, 1, 1
};

OpenGLEffect*	renderpoints			= 0;
OpenGLEffect*	renderlines				= 0;

GLuint			controlvb				= 0;
GLuint			controlib				= 0;
GLuint			controlvao				= 0;

GLuint			splinevb				= 0;
GLuint			splineib				= 0;
GLuint			splinevao				= 0;

GLuint			gridvb					= 0;
GLuint			gridvao					= 0;

GLuint			surfacevb				= 0;
GLuint			surfaceib				= 0;
GLuint			surfacevao				= 0;	// beginning to hate vaos -.-

GLsizei			numsplinevertices		= NUM_SEGMENTS * 2;
GLsizei			numsplineindices		= (numsplinevertices - 1) * 2;
GLsizei			numcontrolvertices		= sizeof(controlpoints) / sizeof(controlpoints[0]);
GLsizei			numcontrolindices		= (numcontrolvertices - 1) * 2;
GLsizei			selectedcontrolpoint	= -1;
float			selectiondx, selectiondy;

array_state<float, 2> cameraangle;

// sample functions
void UpdateControlPoints(float mx, float my);

static float EvalBSplineBasisFunction(int i, int n, float u)
{
	float a, b;

	if( n == 0 )
	{
		if( u >= knot[i] && u < knot[i + 1] )
			return 1;
		else
			return 0;
	}

	if( knot[i + n] == knot[i] )
		a = 0;
	else
		a = (u - knot[i]) / (knot[i + n] - knot[i]) * EvalBSplineBasisFunction(i, n - 1, u);

	if( knot[i + n + 1] == knot[i + 1] )
		b = 0;
	else
		b = (knot[i + n + 1] - u) / (knot[i + n + 1] - knot[i + 1]) * EvalBSplineBasisFunction(i + 1, n - 1, u);

	return a + b;
}

static void EvaluateNURBS(float out[3], float u)
{
	GLsizei k = numcontrolvertices;
	float nom, denom;
	bool valid = false;

	GLVec3Set(out, 0, 0, 0);

	for( GLsizei i = 0; i < k; ++i )
	{
		nom = EvalBSplineBasisFunction(i, 3, u) * weights[i];
		denom = 0;

		for( GLsizei j = 0; j < k; ++j )
			denom += EvalBSplineBasisFunction(j, 3, u) * weights[j];

		if( denom < 1e-5f )
			nom = 0;
		else
		{
			nom /= denom;
			valid = true;
		}

		out[0] += controlpoints[i][0] * nom;
		out[1] += controlpoints[i][1] * nom;
		out[2] += controlpoints[i][2] * nom;
	}

	if( !valid )
		GLVec3Set(out, controlpoints[k - 1][0], controlpoints[k - 1][1], controlpoints[k - 1][2]);
}

static void Tessellate(float (*outvert)[3], unsigned short* outind)
{
	GLsizei numcvs = sizeof(controlpoints) / sizeof(controlpoints[0]);

	for( GLsizei i = 0; i < numsplinevertices; ++i )
		EvaluateNURBS(outvert[i], (float)i / (numsplinevertices - 1));

	for( GLsizei i = 0; i < numsplineindices; i += 2 )
	{
		outind[i] = i / 2;
		outind[i + 1] = i / 2 + 1;
	}
}

static void ConvertToSplineViewport(float& x, float& y)
{
	// first transform [0, w] x [0, h] to [0, 10] x [0, 10]
	float unitscale = 10.0f / screenwidth;

	// then scale it down and offset with 10 pixels
	float vpscale = (float)(screenwidth - 350) / (float)screenwidth;
	float vpoffx = 10.0f;
	float vpoffy = (float)(screenheight - (screenwidth - 350) - 10);

	x = x * unitscale * (1.0f / vpscale) - vpoffx * unitscale * (1.0f / vpscale);
	y = y * unitscale * (1.0f / vpscale) - vpoffy * unitscale * (1.0f / vpscale);
}

void APIENTRY ReportGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userdata)
{
	if( type >= GL_DEBUG_TYPE_ERROR && type <= GL_DEBUG_TYPE_PERFORMANCE )
	{
		if( source == GL_DEBUG_SOURCE_API )
			std::cout << "GL(" << severity << "): ";
		else if( source == GL_DEBUG_SOURCE_SHADER_COMPILER )
			std::cout << "GLSL(" << severity << "): ";
		else
			std::cout << "OTHER(" << severity << "): ";

		std::cout << id << ": " << message << "\n";
	}
}

bool InitScene()
{
	SetWindowText(hwnd, TITLE);
	Quadron::qGLExtensions::QueryFeatures(hdc);

	if( !Quadron::qGLExtensions::ARB_geometry_shader4 )
		return false;

#ifdef _DEBUG
	if( Quadron::qGLExtensions::ARB_debug_output )
	{
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
		glDebugMessageCallback(ReportGLError, 0);
	}
#endif

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	// buffer for a nice and useless grid
	glGenBuffers(1, &gridvb);

	glBindBuffer(GL_ARRAY_BUFFER, gridvb);
	glBufferData(GL_ARRAY_BUFFER, 4 * 11 * 12, 0, GL_STATIC_DRAW);

	float (*vdata)[3] = (float (*)[3])glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

	for( int i = 0; i < 22; i += 2 )
	{
		vdata[i][0] = vdata[i + 1][0] = (float)(i / 2);
		vdata[i][2] = vdata[i + 1][2] = 0;

		vdata[i][1] = 0;
		vdata[i + 1][1] = 10;
	}

	vdata += 22;

	for( int i = 0; i < 22; i += 2 )
	{
		vdata[i][1] = vdata[i + 1][1] = (float)(i / 2);
		vdata[i][2] = vdata[i + 1][2] = 0;

		vdata[i][0] = 0;
		vdata[i + 1][0] = 10;
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);

	glGenVertexArrays(1, &gridvao);
	glBindVertexArray(gridvao);
	{
		glBindBuffer(GL_ARRAY_BUFFER, gridvb);

		glEnableVertexAttribArray(GLDECLUSAGE_POSITION);
		glVertexAttribPointer(GLDECLUSAGE_POSITION, 3, GL_FLOAT, GL_FALSE, 12, 0);
	}
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// buffer for control points
	glGenBuffers(1, &controlvb);
	glGenBuffers(1, &controlib);

	glBindBuffer(GL_ARRAY_BUFFER, controlvb);
	glBufferData(GL_ARRAY_BUFFER, numcontrolvertices * 12, 0, GL_DYNAMIC_DRAW);

	vdata = (float (*)[3])glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

	for( GLsizei i = 0; i < numsplinevertices; ++i )
	{
		vdata[i][0] = controlpoints[i][0];
		vdata[i][1] = controlpoints[i][1];
		vdata[i][2] = controlpoints[i][2];
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, controlib);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numcontrolindices * 2, 0, GL_STATIC_DRAW);

	unsigned short* idata = (unsigned short*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

	for( GLsizei i = 0; i < numcontrolindices; i += 2 )
	{
		idata[i] = i / 2;
		idata[i + 1] = i / 2 + 1;
	}

	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	glGenVertexArrays(1, &controlvao);
	glBindVertexArray(controlvao);
	{
		glBindBuffer(GL_ARRAY_BUFFER, controlvb);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, controlib);

		glEnableVertexAttribArray(GLDECLUSAGE_POSITION);
		glVertexAttribPointer(GLDECLUSAGE_POSITION, 3, GL_FLOAT, GL_FALSE, 12, 0);
	}
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// buffer for spline
	glGenBuffers(1, &splinevb);
	glGenBuffers(1, &splineib);

	glBindBuffer(GL_ARRAY_BUFFER, splinevb);
	glBufferData(GL_ARRAY_BUFFER, numsplinevertices * 12, 0, GL_STATIC_DRAW);

	vdata = (float (*)[3])glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, splineib);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numsplineindices * 2, 0, GL_STATIC_DRAW);

	idata = (unsigned short*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

	Tessellate(vdata, idata);

	glUnmapBuffer(GL_ARRAY_BUFFER);
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	glGenVertexArrays(1, &splinevao);
	glBindVertexArray(splinevao);
	{
		glBindBuffer(GL_ARRAY_BUFFER, splinevb);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, splineib);

		glEnableVertexAttribArray(GLDECLUSAGE_POSITION);
		glVertexAttribPointer(GLDECLUSAGE_POSITION, 3, GL_FLOAT, GL_FALSE, 12, 0);
	}
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// load effects
	if( !GLCreateEffectFromFile("../media/shadersGL/color.vert", "../media/shadersGL/renderpoints.geom", "../media/shadersGL/color.frag", &renderpoints) )
	{
		MYERROR("Could not load point renderer shader");
		return false;
	}

	if( !GLCreateEffectFromFile("../media/shadersGL/color.vert", "../media/shadersGL/renderlines.geom", "../media/shadersGL/color.frag", &renderlines) )
	{
		MYERROR("Could not load line renderer shader");
		return false;
	}

	float angles[2] = { -0.25f, 0.7f };
	cameraangle = angles;

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_DELETE(renderpoints);
	SAFE_DELETE(renderlines);

	if( splinevao )
		glDeleteVertexArrays(1, &splinevao);

	if( controlvao )
		glDeleteVertexArrays(1, &controlvao);

	if( gridvao )
		glDeleteVertexArrays(1, &gridvao);

	if( gridvb )
		glDeleteBuffers(1, &gridvb);

	if( controlvb )
		glDeleteBuffers(1, &controlvb);

	if( controlib )
		glDeleteBuffers(1, &controlib);

	if( splinevb )
		glDeleteBuffers(1, &splinevb);

	if( splineib )
		glDeleteBuffers(1, &splineib);

	GLKillAnyRogueObject();
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
}
//*************************************************************************************************************
void UpdateControlPoints(float mx, float my)
{
	float	sspx = mx;
	float	sspy = screenheight - my - 1;
	float	dist;
	float	radius = 10.0f / (screenheight / 10);

	ConvertToSplineViewport(sspx, sspy);

	// convert to spline coordinate system (10x10)
	//sspx = mx / (screenheight / 10);
	//sspy = (screenheight - my - 1) / (screenheight / 10);

	if( selectedcontrolpoint == -1 )
	{
		for( GLsizei i = 0; i < numcontrolvertices; ++i )
		{
			selectiondx = controlpoints[i][0] - sspx;
			selectiondy = controlpoints[i][1] - sspy;
			dist = selectiondx * selectiondx + selectiondy * selectiondy;

			if( dist < radius * radius )
			{
				selectedcontrolpoint = i;
				break;
			}
		}
	}

	if( selectedcontrolpoint > -1 && selectedcontrolpoint < numcontrolvertices )
	{
		controlpoints[selectedcontrolpoint][0] = GLMin<float>(GLMax<float>(selectiondx + sspx, 0), 10);
		controlpoints[selectedcontrolpoint][1] = GLMin<float>(GLMax<float>(selectiondy + sspy, 0), 10);

		glBindBuffer(GL_ARRAY_BUFFER, controlvb);

		float* data = (float*)glMapBufferRange(
			GL_ARRAY_BUFFER, selectedcontrolpoint * 3 * sizeof(float),
			3 * sizeof(float), GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_RANGE_BIT);

		data[0] = controlpoints[selectedcontrolpoint][0];
		data[1] = controlpoints[selectedcontrolpoint][1];
		data[2] = controlpoints[selectedcontrolpoint][2];

		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}
//*************************************************************************************************************
void Update(float delta)
{
	cameraangle.prev[0] = cameraangle.curr[0];
	cameraangle.prev[1] = cameraangle.curr[1];

	if( mousedown == 1 )
	{
		UpdateControlPoints((float)mousex, (float)mousey);

		cameraangle.curr[0] += mousedx * 0.004f;
		cameraangle.curr[1] += mousedy * 0.004f;
	}
	else
		selectedcontrolpoint = -1;

	// clamp to [-pi, pi]
	if( cameraangle.curr[1] >= 1.5f )
		cameraangle.curr[1] = 1.5f;

	if( cameraangle.curr[1] <= -1.5f )
		cameraangle.curr[1] = -1.5f;
}
//*************************************************************************************************************
void Render(float alpha, float elapsedtime)
{
	OpenGLColor	grcolor(0xffdddddd);
	OpenGLColor	cvcolor(0xff7470ff);
	OpenGLColor	splinecolor(0xff000000);

	float		world[16];
	float		proj[16];
	float		pointsize[2] = { 10.0f / screenwidth, 10.0f / screenheight };
	float		grthickness[2] = { 1.5f / screenwidth, 1.5f / screenheight };
	float		cvthickness[2] = { 2.0f / screenwidth, 2.0f / screenheight };
	float		spthickness[2] = { 3.0f / screenwidth, 3.0f / screenheight };
	float		spviewport[] = { 0, 0, (float)screenwidth, (float)screenheight };

	// play with ortho matrix instead of viewport (line thickness remains constant)
	ConvertToSplineViewport(spviewport[0], spviewport[1]);
	ConvertToSplineViewport(spviewport[2], spviewport[3]);

	GLMatrixIdentity(world);
	GLMatrixOrthoRH(proj, spviewport[0], spviewport[2], spviewport[1], spviewport[3], -1, 1);

	glViewport(0, 0, screenwidth, screenheight);
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	renderlines->SetMatrix("matViewProj", proj);
	renderlines->SetMatrix("matWorld", world);
	renderlines->SetVector("color", &grcolor.r);
	renderlines->SetVector("lineThickness", grthickness);

	renderlines->Begin();
	{
		glBindVertexArray(gridvao);
		glDrawArrays(GL_LINES, 0, 44);

		renderlines->SetVector("color", &cvcolor.r);
		renderlines->SetVector("lineThickness", cvthickness);
		renderlines->CommitChanges();

		glBindVertexArray(controlvao);
		glDrawElements(GL_LINES, numcontrolindices, GL_UNSIGNED_SHORT, 0);

		renderlines->SetVector("lineThickness", spthickness);
		renderlines->SetVector("color", &splinecolor.r);
		renderlines->CommitChanges();

		glBindVertexArray(splinevao);
		glDrawElements(GL_LINES, numsplineindices, GL_UNSIGNED_SHORT, 0);

		glBindVertexArray(0);
	}
	renderlines->End();

	renderpoints->SetMatrix("matViewProj", proj);
	renderpoints->SetMatrix("matWorld", world);
	renderpoints->SetVector("color", &cvcolor.r);
	renderpoints->SetVector("pointSize", pointsize);

	renderpoints->Begin();
	{
		glBindVertexArray(controlvao);
		glDrawArrays(GL_POINTS, 0, numcontrolvertices);
		glBindVertexArray(0);
	}
	renderpoints->End();

	// render surface in a smaller window
	glEnable(GL_SCISSOR_TEST);
	glScissor(screenwidth - 330, screenheight - 250, 320, 240);

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);

	glEnable(GL_DEPTH_TEST);
	glViewport(screenwidth - 330, screenheight - 250, 320, 240);

	// TODO: draw surface

#ifdef _DEBUG
	// check errors
	GLenum err = glGetError();

	if( err != GL_NO_ERROR )
		std::cout << "Error\n";
#endif

	SwapBuffers(hdc);
}
//*************************************************************************************************************
