//*************************************************************************************************************
#include <Windows.h>
#include <iostream>

#include "../common/glext.h"

// TODO:
// - normalok, shading
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

const GLuint numcontrolvertices		= sizeof(controlpoints) / sizeof(controlpoints[0]);
const GLuint numcontrolindices		= (numcontrolvertices - 1) * 2;
const GLuint numknots				= sizeof(knot) / sizeof(knot[0]);
const GLuint numsplinevertices		= NUM_SEGMENTS + 1;
const GLuint numsplineindices		= (numsplinevertices - 1) * 2;
const GLuint numsurfacevertices		= numsplinevertices * numsplinevertices;
const GLuint numsurfaceindices		= NUM_SEGMENTS * NUM_SEGMENTS * 6;

OpenGLEffect*	renderpoints			= 0;
OpenGLEffect*	renderlines				= 0;
OpenGLEffect*	rendersurface			= 0;
OpenGLMesh*		supportlines			= 0;
OpenGLMesh*		curve					= 0;
OpenGLMesh*		surface					= 0;
GLsizei			selectedcontrolpoint	= -1;
float			selectiondx, selectiondy;

array_state<float, 2> cameraangle;

// sample functions
void UpdateControlPoints(float mx, float my);

static float CalculateCoeff(GLuint i, int n, int k, float u)
{
	float kin1	= knot[i + n + 1];
	float ki1	= knot[i + 1];
	float ki	= knot[i];
	float kin	= knot[i + n];
	float a		= 0;
	float b		= 0;

	if( n == 0 )
	{
		if( knot[i] <= u && u < knot[i + 1] || (u == 1.0f && i >= numcontrolvertices - 1) )
			return 1.0f;
	}
	else if( k == 0 )
	{
		if( kin1 != ki1 )
			a = (kin1 * CalculateCoeff(i + 1, n - 1, 0, u)) / (kin1 - ki1);

		if( ki != kin )
			b = (ki * CalculateCoeff(i, n - 1, 0, u) / (kin - ki));
	}
	else if( k == n )
	{
		if( ki != kin )
			a = CalculateCoeff(i, n - 1, n - 1, u) / (kin - ki);

		if( kin1 != ki1 )
			b = CalculateCoeff(i + 1, n - 1, n - 1, u) / (kin1 - ki1);
	}
	else if( n > k )
	{
		if( ki != kin )
			a = (CalculateCoeff(i, n - 1, k - 1, u) - ki * CalculateCoeff(i, n - 1, k, u)) / (kin - ki);

		if( kin1 != ki1 )
			b = (CalculateCoeff(i + 1, n - 1, k - 1, u) - kin1 * CalculateCoeff(i + 1, n - 1, k, u)) / (kin1 - ki1);
	}

	return a - b;
}

static void TessellateCurve(float (*outvert)[3], unsigned short* outind)
{
	typedef float coefftable[4][4];

	coefftable*	coeffs = new coefftable[numknots - 1];
	GLuint		span;
	GLuint		cp;
	float		u;
	float		nom, denom;

	// precalculate basis function coefficients
	for( span = 0; span < numknots - 1; ++span )
	{
		for( GLuint k = 0; k < 4; ++k )
		{
			if( span >= 3 - k && span < numcontrolvertices - k + 3 )
			{
				cp = span - (3 - k);

				coeffs[span][k][0] = CalculateCoeff(cp, 3, 0, knot[span]);
				coeffs[span][k][1] = CalculateCoeff(cp, 3, 1, knot[span]);
				coeffs[span][k][2] = CalculateCoeff(cp, 3, 2, knot[span]);
				coeffs[span][k][3] = CalculateCoeff(cp, 3, 3, knot[span]);
			}
			else
			{
				coeffs[span][k][0] = 0;
				coeffs[span][k][1] = 0;
				coeffs[span][k][2] = 0;
				coeffs[span][k][3] = 0;
			}
		}
	}

	// fill vertex buffer
	for( GLuint i = 0; i < numsplinevertices; ++i )
	{
		u = (float)i / (numsplinevertices - 1);
		GLVec3Set(outvert[i], 0, 0, 0);

		// find span
		for( span = 0; span < numknots - 1; ++span )
		{
			if( knot[span] <= u && u < knot[span + 1] )
				break;
		}

		if( span >= numknots - 1 )
			span = numknots - 2;

		const coefftable& table = coeffs[span];

		// calculate normalization factor
		denom = 0;

		for( GLuint k = 0; k < 4; ++k )
		{
			const float (&coeffs)[4] = table[k];
			denom += (coeffs[0] + coeffs[1] * u + coeffs[2] * u * u + coeffs[3] * u * u * u);
		}

		// sum contributions
		if( denom > 1e-5f )
		{
			denom = 1.0f / denom;

			for( GLuint k = 0; k < 4; ++k )
			{
				const float (&coeffs)[4] = table[k];
				nom = (coeffs[0] + coeffs[1] * u + coeffs[2] * u * u + coeffs[3] * u * u * u);

				cp = span - (3 - k) % numcontrolvertices;
				nom = nom * weights[cp] * denom;

				outvert[i][0] += controlpoints[cp][0] * nom;
				outvert[i][1] += controlpoints[cp][1] * nom;
				outvert[i][2] += controlpoints[cp][2] * nom;
			}
		}
	}

	delete[] coeffs;

	// fill index buffer
	for( GLuint i = 0; i < numsplineindices; i += 2 )
	{
		outind[i] = i / 2;
		outind[i + 1] = i / 2 + 1;
	}
}

static float EvalBSplineBasisFunction(int i, int n, float u)
{
	float c30 = CalculateCoeff(i, n, 0, u);
	float c31 = CalculateCoeff(i, n, 1, u);
	float c32 = CalculateCoeff(i, n, 2, u);
	float c33 = CalculateCoeff(i, n, 3, u);

	return ((c33 * u + c32) * u + c31) * u + c30;
}

static void EvaluateNURBSSurface(float out[3], float u, float v)
{
	GLuint k = numcontrolvertices;
	GLuint l = numcontrolvertices;
	float nom, denom = 0;

	GLVec3Set(out, 0, 0, 0);

	for( GLuint p = 0; p < k; ++p )
	{
		for( GLuint q = 0; q < l; ++q )
		{
			denom +=
				EvalBSplineBasisFunction(p, 3, u) *
				EvalBSplineBasisFunction(q, 3, v) * weights[p] * weights[q];
		}
	}

	if( denom > 1e-5f )
	{
		for( GLuint i = 0; i < k; ++i )
		{
			for( GLuint j = 0; j < l; ++j )
			{
				nom =
					EvalBSplineBasisFunction(i, 3, u) *
					EvalBSplineBasisFunction(j, 3, v) * weights[i] * weights[j];

				nom /= denom;

				out[0] += controlpoints[i][0] * nom;
				out[1] += (controlpoints[i][1] + controlpoints[j][1]) * 0.5f * nom;
				out[2] += controlpoints[j][0] * nom;
			}
		}
	}
}

static void TessellateSurface(float (*outvert)[3], unsigned int* outind)
{
	GLsizei numcvs = sizeof(controlpoints) / sizeof(controlpoints[0]);
	GLsizei tile, row, col;

	for( GLuint i = 0; i < numsplinevertices; ++i )
	{
		for( GLuint j = 0; j < numsplinevertices; ++j )
		{
			EvaluateNURBSSurface(
				outvert[i * numsplinevertices + j],
				(float)i / (numsplinevertices - 1),
				(float)j / (numsplinevertices - 1));
		}
	}

	for( GLuint i = 0; i < numsurfaceindices; i += 6 )
	{
		tile = i / 6;
		row = tile / NUM_SEGMENTS;
		col = tile % NUM_SEGMENTS;

		outind[i + 0] = outind[i + 3]	= row * numsplinevertices + col;
		outind[i + 2]					= (row + 1) * numsplinevertices + col;
		outind[i + 1] = outind[i + 5]	= (row + 1) * numsplinevertices + col + 1;
		outind[i + 4]					= row * numsplinevertices + col + 1;
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
	float		(*vdata)[3] = 0;
	GLushort*	idata = 0;
	GLuint*		idata32 = 0;

	OpenGLVertexElement decl[] =
	{
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

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

	// create grid & control poly
	if( !GLCreateMesh(44 + numcontrolvertices, numcontrolindices, GLMESH_DYNAMIC, decl, &supportlines) )
	{
		MYERROR("Could not create mesh");
		return false;
	}

	supportlines->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
	supportlines->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata);

	for( int i = 0; i < 22; i += 2 )
	{
		vdata[i][0] = vdata[i + 1][0] = (float)(i / 2);
		vdata[i][2] = vdata[i + 1][2] = 0;

		vdata[i][1] = 0;
		vdata[i + 1][1] = 10;

		vdata[i + 22][1] = vdata[i + 23][1] = (float)(i / 2);
		vdata[i + 22][2] = vdata[i + 23][2] = 0;

		vdata[i + 22][0] = 0;
		vdata[i + 23][0] = 10;
	}

	vdata += 44;

	for( GLsizei i = 0; i < numcontrolvertices; ++i )
	{
		vdata[i][0] = controlpoints[i][0];
		vdata[i][1] = controlpoints[i][1];
		vdata[i][2] = controlpoints[i][2];
	}

	for( GLsizei i = 0; i < numcontrolindices; i += 2 )
	{
		idata[i] = 44 + i / 2;
		idata[i + 1] = 44 + i / 2 + 1;
	}

	supportlines->UnlockIndexBuffer();
	supportlines->UnlockVertexBuffer();

	OpenGLAttributeRange table[] =
	{
		{ GLPT_LINELIST, 0, 0, 0, 0, 44 },
		{ GLPT_LINELIST, 1, 0, numcontrolindices, 44, numcontrolvertices }
	};

	supportlines->SetAttributeTable(table, 2);

	// create spline mesh
	if( !GLCreateMesh(numsplinevertices, numsplineindices, 0, decl, &curve) )
	{
		MYERROR("Could not create curve");
		return false;
	}

	curve->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
	curve->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata);

	TessellateCurve(vdata, idata);

	curve->UnlockIndexBuffer();
	curve->UnlockVertexBuffer();

	curve->GetAttributeTable()->PrimitiveType = GLPT_LINELIST;

	// create surface
	if( !GLCreateMesh(numsurfacevertices, numsurfaceindices, GLMESH_32BIT, decl, &surface) )
	{
		MYERROR("Could not create surface");
		return false;
	}

	surface->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
	surface->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata32);

	TessellateSurface(vdata, idata32);

	surface->UnlockIndexBuffer();
	surface->UnlockVertexBuffer();

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

	if( !GLCreateEffectFromFile("../media/shadersGL/rendersurface.vert", 0, "../media/shadersGL/rendersurface.frag", &rendersurface) )
	{
		MYERROR("Could not load surface renderer shader");
		return false;
	}

	float angles[2] = { 0, 0 }; //{ -0.25f, 0.7f };
	cameraangle = angles;

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_DELETE(renderpoints);
	SAFE_DELETE(renderlines);
	SAFE_DELETE(rendersurface);
	SAFE_DELETE(supportlines);
	SAFE_DELETE(surface);
	SAFE_DELETE(curve);

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

		float* data = 0;

		supportlines->LockVertexBuffer(
			(44 + selectedcontrolpoint) * supportlines->GetNumBytesPerVertex(),
			supportlines->GetNumBytesPerVertex(), GLLOCK_DISCARD, (void**)&data);

		data[0] = controlpoints[selectedcontrolpoint][0];
		data[1] = controlpoints[selectedcontrolpoint][1];
		data[2] = controlpoints[selectedcontrolpoint][2];

		supportlines->UnlockVertexBuffer();
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

		if( (mousex >= screenwidth - 330 && mousex <= screenwidth - 10) &&
			(mousey >= 10 && mousey <= 260) )
		{
			cameraangle.curr[0] += mousedx * 0.004f;
			cameraangle.curr[1] += mousedy * 0.004f;
		}
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
	float		view[16];
	float		proj[16];
	float		viewproj[16];

	float		pointsize[2]	= { 10.0f / screenwidth, 10.0f / screenheight };
	float		grthickness[2]	= { 1.5f / screenwidth, 1.5f / screenheight };
	float		cvthickness[2]	= { 2.0f / screenwidth, 2.0f / screenheight };
	float		spthickness[2]	= { 3.0f / screenwidth, 3.0f / screenheight };
	float		spviewport[]	= { 0, 0, (float)screenwidth, (float)screenheight };

	float		eye[3]			= { 5, 5, 15 };
	float		look[3]			= { 5, 5, 5 };
	float		up[3]			= { 0, 1, 0 };
	float		fwd[3];
	float		orient[2];

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
		supportlines->DrawSubset(0);

		renderlines->SetVector("color", &cvcolor.r);
		renderlines->SetVector("lineThickness", cvthickness);
		renderlines->CommitChanges();

		supportlines->DrawSubset(1);

		renderlines->SetVector("lineThickness", spthickness);
		renderlines->SetVector("color", &splinecolor.r);
		renderlines->CommitChanges();

		curve->DrawSubset(0);
	}
	renderlines->End();

	renderpoints->SetMatrix("matViewProj", proj);
	renderpoints->SetMatrix("matWorld", world);
	renderpoints->SetVector("color", &cvcolor.r);
	renderpoints->SetVector("pointSize", pointsize);

	renderpoints->Begin();
	{
		glBindVertexArray(supportlines->GetVertexLayout());
		glDrawArrays(GL_POINTS, 44, numcontrolvertices);
	}
	renderpoints->End();

	// render surface in a smaller viewport
	glEnable(GL_SCISSOR_TEST);
	glScissor(screenwidth - 330, screenheight - 250, 320, 240);

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);

	glEnable(GL_DEPTH_TEST);
	glViewport(screenwidth - 330, screenheight - 250, 320, 240);

	cameraangle.smooth(orient, alpha);

	GLVec3Subtract(fwd, look, eye);
	GLMatrixRotationYawPitchRoll(view, orient[0], orient[1], 0);
	GLVec3Transform(fwd, fwd, view);
	GLVec3Subtract(eye, look, fwd);

	GLMatrixPerspectiveRH(proj, M_PI / 3, 4.0f / 3.0f, 0.1f, 50.0f);
	GLMatrixLookAtRH(view, eye, look, up);
	GLMatrixMultiply(viewproj, view, proj);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	rendersurface->SetMatrix("matViewProj", viewproj);
	rendersurface->SetMatrix("matWorld", world);

	rendersurface->Begin();
	{
		surface->DrawSubset(0);
	}
	rendersurface->End();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#ifdef _DEBUG
	// check errors
	GLenum err = glGetError();

	if( err != GL_NO_ERROR )
		std::cout << "Error\n";
#endif

	SwapBuffers(hdc);
}
//*************************************************************************************************************
