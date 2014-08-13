//*************************************************************************************************************
#include <Windows.h>
#include <GdiPlus.h>
#include <iostream>
#include <sstream>

#include "../common/glext.h"

// TODO:
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

// sample structures
struct SurfaceVertex
{
	float pos[3];
	float norm[3];
};

// curve data
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

float knots[] =
{
	0, 0, 0, 0, 0.4f, 0.4f, 0.4f, 1, 1, 1, 1
	//0, 0, 0, 0, 0.25f, 0.5f, 0.75f, 1, 1, 1, 1
	//0, 0, 0, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 1, 1, 1
	//0, 0, 0.125f, 0.25f, 0.325f, 0.5f, 0.625f, 0.75f, 0.875f, 1, 1
	//0, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1
	//0.2f, 0.3f, 0.35f, 0.4f, 0.45f, 0.5f, 0.55f, 0.6f, 0.65f, 0.7f, 0.8f
};

const GLuint numcontrolvertices				= sizeof(controlpoints) / sizeof(controlpoints[0]);
const GLuint numcontrolindices				= (numcontrolvertices - 1) * 2;
const GLuint numknots						= sizeof(knots) / sizeof(knots[0]);

// important variables for tessellation
const GLuint numsplinevertices				= NUM_SEGMENTS + 1;
const GLuint numsurfacevertices				= numsplinevertices * numsplinevertices;
GLuint numsplineindices						= (numsplinevertices - 1) * 2;
GLuint numsurfaceindices					= (numsplinevertices - 1) * (numsplinevertices - 1) * 6;

// sample variables
OpenGLScreenQuad*	screenquad				= 0;
OpenGLEffect*		renderpoints			= 0;
OpenGLEffect*		renderlines				= 0;
OpenGLEffect*		rendersurface			= 0;
OpenGLEffect*		basic2D					= 0;
OpenGLMesh*			supportlines			= 0;
OpenGLMesh*			curve					= 0;
OpenGLMesh*			surface					= 0;
GLuint				text1					= 0;
GLsizei				selectedcontrolpoint	= -1;
float				selectiondx, selectiondy;
bool				wireframe				= false;
bool				fullscreen				= false;

array_state<float, 2> cameraangle;

// sample functions
void UpdateControlPoints(float mx, float my);

static float CalculateCoeff(GLuint i, int n, int k, GLuint span)
{
	float kin1	= knots[i + n + 1];
	float ki1	= knots[i + 1];
	float ki	= knots[i];
	float kin	= knots[i + n];
	float a		= 0;
	float b		= 0;

	if( n == 0 )
	{
		if( i == span ) // || i > lastspan
			return 1.0f;
	}
	else if( k == 0 )
	{
		if( kin1 != ki1 )
			a = (kin1 * CalculateCoeff(i + 1, n - 1, 0, span)) / (kin1 - ki1);

		if( ki != kin )
			b = (ki * CalculateCoeff(i, n - 1, 0, span) / (kin - ki));
	}
	else if( k == n )
	{
		if( ki != kin )
			a = CalculateCoeff(i, n - 1, n - 1, span) / (kin - ki);

		if( kin1 != ki1 )
			b = CalculateCoeff(i + 1, n - 1, n - 1, span) / (kin1 - ki1);
	}
	else if( n > k )
	{
		if( ki != kin )
			a = (CalculateCoeff(i, n - 1, k - 1, span) - ki * CalculateCoeff(i, n - 1, k, span)) / (kin - ki);

		if( kin1 != ki1 )
			b = (CalculateCoeff(i + 1, n - 1, k - 1, span) - kin1 * CalculateCoeff(i + 1, n - 1, k, span)) / (kin1 - ki1);
	}

	return a - b;
}

static void TessellateCurve(float (*outvert)[3], unsigned short* outind)
{
	typedef float CoeffTable[4][4];

	CoeffTable*	coeffs = new CoeffTable[numknots - 1];
	GLuint		verticeswritten = 0;
	GLuint		lastspan;
	GLuint		span;
	GLuint		cp;
	float		u;
	float		minweight;
	float		nom, denom;
	float		poly[4];

	// find last span
	lastspan = numknots - 2;

	while( knots[lastspan] == knots[numknots - 1] )
		--lastspan;

	// find minimum weight
	minweight = weights[0];

	for( GLuint i = 1; i < numcontrolvertices; ++i )
		minweight = GLMin<float>(minweight, weights[i]);

	minweight -= 1e-3f;

	// precalculate basis function coefficients
	for( span = 0; span < numknots - 1; ++span )
	{
		for( GLuint k = 0; k < 4; ++k )
		{
			if( span >= k && span - k < numcontrolvertices )
			{
				cp = span - k;

				coeffs[span][k][0] = CalculateCoeff(cp, 3, 0, span); // 1
				coeffs[span][k][1] = CalculateCoeff(cp, 3, 1, span); // u
				coeffs[span][k][2] = CalculateCoeff(cp, 3, 2, span); // u2
				coeffs[span][k][3] = CalculateCoeff(cp, 3, 3, span); // u3
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
	for( GLuint i = 0; i <= NUM_SEGMENTS; ++i )
	{
		u = (float)i / NUM_SEGMENTS;
		u = knots[0] * (1 - u) + knots[numknots - 1] * u;

		poly[0] = 1;
		poly[1] = u;
		poly[2] = u * u;
		poly[3] = u * u * u;

		// find span
		for( span = 0; span < numknots - 1; ++span )
		{
			if( (knots[span] <= u && u < knots[span + 1]) || (span == lastspan) )
				break;
		}

		const CoeffTable& table = coeffs[span];

		// calculate normalization factor
		denom = 0;

		for( GLuint k = 0; k < 4; ++k )
		{
			cp = (span - k) % numcontrolvertices;
			denom += GLVec4Dot(table[k], poly) * weights[cp];
		}

		// sum contributions
		if( denom > minweight )
		{
			GLVec3Set(outvert[verticeswritten], 0, 0, 0);
			denom = 1.0f / denom;

			for( GLuint k = 0; k < 4; ++k )
			{
				nom = GLVec4Dot(table[k], poly);

				cp = (span - k) % numcontrolvertices;
				nom = nom * weights[cp] * denom;

				outvert[verticeswritten][0] += controlpoints[cp][0] * nom;
				outvert[verticeswritten][1] += controlpoints[cp][1] * nom;
				outvert[verticeswritten][2] += controlpoints[cp][2] * nom;
			}

			++verticeswritten;
		}
	}

	delete[] coeffs;

	// fill index buffer
	numsplineindices = (verticeswritten - 1) * 2;

	for( GLuint i = 0; i < numsplineindices; i += 2 )
	{
		outind[i] = i / 2;
		outind[i + 1] = i / 2 + 1;
	}
}

static void TessellateSurface(SurfaceVertex* outvert, unsigned int* outind)
{
	typedef float CoeffTable[4][4];

	CoeffTable*	ucoeffs = new CoeffTable[numknots - 1];
	CoeffTable*	vcoeffs = ucoeffs; // lucky
	GLuint		verticeswritten = 0;
	GLuint		rowswritten = 0;
	GLuint		uspan, vspan;
	GLuint		cpu, cpv;
	GLuint		lastuspan, lastvspan;
	GLsizei		tile, row, col;
	float		u, v;
	float		minuweight, minvweight;
	float		nom, denom;
	float		dUnom;
	float		dVnom;
	float		ones[4] = { 1, 1, 1, 1 };
	float		uparts[4];
	float		upoly[4], vpoly[4];
	float		dUpoly[3], dVpoly[3];
	float		tangent[3], bitangent[3];
	float		cp[3];

	// find last span
	lastuspan = numknots - 2;

	while( knots[lastuspan] == knots[numknots - 1] )
		--lastuspan;

	lastvspan = lastuspan; // lucky

	// find minimum weight
	minuweight = weights[0];

	for( GLuint i = 1; i < numcontrolvertices; ++i )
		minuweight = GLMin<float>(minuweight, weights[i]);

	minuweight -= 1e-3f;
	minvweight = minuweight; // lucky

	// precalculate basis function coefficients
	for( uspan = 0; uspan < numknots - 1; ++uspan )
	{
		for( GLuint k = 0; k < 4; ++k )
		{
			if( uspan >= k && uspan - k < numcontrolvertices )
			{
				cpu = uspan - k;

				ucoeffs[uspan][k][0] = CalculateCoeff(cpu, 3, 0, uspan);
				ucoeffs[uspan][k][1] = CalculateCoeff(cpu, 3, 1, uspan);
				ucoeffs[uspan][k][2] = CalculateCoeff(cpu, 3, 2, uspan);
				ucoeffs[uspan][k][3] = CalculateCoeff(cpu, 3, 3, uspan);
			}
			else
			{
				ucoeffs[uspan][k][0] = 0;
				ucoeffs[uspan][k][1] = 0;
				ucoeffs[uspan][k][2] = 0;
				ucoeffs[uspan][k][3] = 0;
			}
		}
	}

	for( GLuint i = 0; i <= NUM_SEGMENTS; ++i )
	{
		u = (float)i / NUM_SEGMENTS;
		u = knots[0] * (1 - u) + knots[numknots - 1] * u;

		upoly[0] = 1;
		upoly[1] = u;
		upoly[2] = u * u;
		upoly[3] = u * u * u;

		dUpoly[0] = 1;
		dUpoly[1] = 2 * u;
		dUpoly[2] = 3 * u * u;

		// find u span
		for( uspan = 0; uspan < numknots - 1; ++uspan )
		{
			if( (knots[uspan] <= u && u < knots[uspan + 1]) || (uspan == lastuspan) )
				break;
		}

		const CoeffTable& utable = ucoeffs[uspan];

		// fast cull: test u part of denom
		for( GLuint k = 0; k < 4; ++k )
		{
			cpu = (uspan - k) % numcontrolvertices;
			uparts[k] = GLVec4Dot(utable[k], upoly) * weights[cpu];
		}

		denom = uparts[0] + uparts[1] + uparts[2] + uparts[3]; // dot(uparts, vec4(1.0))

		if( denom > minuweight )
		{
			for( GLuint j = 0; j <= NUM_SEGMENTS; ++j )
			{
				v = (float)j / NUM_SEGMENTS;
				v = knots[0] * (1 - v) + knots[numknots - 1] * v;

				vpoly[0] = 1;
				vpoly[1] = v;
				vpoly[2] = v * v;
				vpoly[3] = v * v * v;

				dVpoly[0] = 1;
				dVpoly[1] = 2 * v;
				dVpoly[2] = 3 * v * v;

				// find v span
				for( vspan = 0; vspan < numknots - 1; ++vspan )
				{
					if( (knots[vspan] <= v && v < knots[vspan + 1]) || (vspan == lastvspan) )
						break;
				}

				const CoeffTable& vtable = vcoeffs[vspan];

				denom = 0;

				for( GLuint l = 0; l < 4; ++l )
				{
					cpv = (vspan - l) % numcontrolvertices;
					denom += GLVec4Dot(vtable[l], vpoly) * weights[cpv];
				}

				// sum contributions
				if( denom > minvweight )
				{
					// calculate normalization factor
					denom = uparts[0] * denom + uparts[1] * denom + uparts[2] * denom + uparts[3] * denom; // dot(uparts, vec4(denom));
					denom = 1.0f / denom;

					GLVec3Set(outvert[verticeswritten].pos, 0, 0, 0);
					GLVec3Set(outvert[verticeswritten].norm, 0, 1, 0);
					GLVec3Set(tangent, 0, 0, 0);
					GLVec3Set(bitangent, 0, 0, 0);

					// sum contributions
					for( GLuint k = 0; k < 4; ++k )
					{
						for( GLuint l = 0; l < 4; ++l )
						{
							nom = GLVec4Dot(utable[k], upoly) * GLVec4Dot(vtable[l], vpoly);
							dUnom = GLVec3Dot(&utable[k][1], dUpoly) * GLVec4Dot(vtable[l], vpoly);
							dVnom = GLVec4Dot(utable[k], upoly) * GLVec3Dot(&vtable[l][1], dVpoly);

							cpu = (uspan - k) % numcontrolvertices;
							cpv = (vspan - l) % numcontrolvertices;

							nom = nom * weights[cpu] * weights[cpv] * denom;
							dUnom = dUnom * weights[cpu] * weights[cpv] * denom;
							dVnom = dVnom * weights[cpu] * weights[cpv] * denom;

							// it's just an idea, but works
							cp[0] = controlpoints[cpu][0];
							cp[1] = (controlpoints[cpu][1] + controlpoints[cpv][1]) * 0.5f;
							cp[2] = controlpoints[cpv][0];

							outvert[verticeswritten].pos[0] += cp[0] * nom;
							outvert[verticeswritten].pos[1] += cp[1] * nom;
							outvert[verticeswritten].pos[2] += cp[2] * nom;

							tangent[0] += cp[0] * dUnom;
							tangent[1] += cp[1] * dUnom;
							tangent[2] += cp[2] * dUnom;

							bitangent[0] += cp[0] * dVnom;
							bitangent[1] += cp[1] * dVnom;
							bitangent[2] += cp[2] * dVnom;
						}
					}

					// calculate normal
					GLVec3Cross(outvert[verticeswritten].norm, bitangent, tangent);

					++verticeswritten;
				}
			}

			++rowswritten;
		}
	}

	if( vcoeffs != ucoeffs )
		delete[] vcoeffs;

	delete[] ucoeffs;

	// fill index buffer
	GLuint colswritten = verticeswritten / rowswritten;

	numsurfaceindices = (rowswritten - 1) * (colswritten - 1) * 6;

	for( GLuint i = 0; i < numsurfaceindices; i += 6 )
	{
		tile = i / 6;
		col = tile / (colswritten - 1);
		row = tile % (rowswritten - 1);

		outind[i + 0] = outind[i + 3]	= row * colswritten + col;
		outind[i + 2]					= (row + 1) * colswritten + col;
		outind[i + 1] = outind[i + 5]	= (row + 1) * colswritten + col + 1;
		outind[i + 4]					= row * colswritten + col + 1;
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

static void UpdateText()
{
	std::stringstream ss;

	ss.precision(1);
	ss << "Knot vector is:  { " << knots[0];

	for( GLuint i = 1; i < numknots; ++i )
		ss << ", " << knots[i];

	ss << " }\nWeights are:     { " << weights[0];

	for( GLuint i = 1; i < numcontrolvertices; ++i )
		ss << ", " << weights[i];

	ss << " }\n\nC - cycle knots  W - wireframe  F - full window  +/- tessellation level";
	GLRenderTextEx(ss.str(), text1, screenwidth, screenheight - (screenwidth - 330), L"Calibri", false, Gdiplus::FontStyleBold, 25);
}

static void APIENTRY ReportGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userdata)
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
	SurfaceVertex*	svdata = 0;
	float			(*vdata)[3] = 0;
	GLushort*		idata = 0;
	GLuint*			idata32 = 0;

	OpenGLVertexElement decl[] =
	{
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	OpenGLVertexElement decl2[] =
	{
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0, 12, GLDECLTYPE_FLOAT3, GLDECLUSAGE_NORMAL, 0 },
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

	OpenGLAttributeRange* subset0 = curve->GetAttributeTable();

	subset0->PrimitiveType = GLPT_LINELIST;
	subset0->IndexCount = numsplineindices;

	// create surface
	if( !GLCreateMesh(numsurfacevertices, numsurfaceindices, GLMESH_32BIT, decl2, &surface) )
	{
		MYERROR("Could not create surface");
		return false;
	}

	surface->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&svdata);
	surface->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata32);

	TessellateSurface(svdata, idata32);

	surface->UnlockIndexBuffer();
	surface->UnlockVertexBuffer();

	subset0 = surface->GetAttributeTable();

	subset0->PrimitiveType = GLPT_TRIANGLELIST;
	subset0->IndexCount = numsurfaceindices;

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

	if( !GLCreateEffectFromFile("../media/shadersGL/basic2D.vert", 0, "../media/shadersGL/basic2D.frag", &basic2D) )
	{
		MYERROR("Could not load basic 2D shader");
		return false;
	}

	screenquad = new OpenGLScreenQuad();

	// text
	GLCreateTexture(screenwidth, screenheight - (screenwidth - 330), 1, GLFMT_A8R8G8B8, &text1);
	UpdateText();

	float angles[2] = { M_PI / 2, 0.5f };
	cameraangle = angles;

	return true;
}
//*************************************************************************************************************
void UninitScene()
{
	SAFE_DELETE(renderpoints);
	SAFE_DELETE(renderlines);
	SAFE_DELETE(rendersurface);
	SAFE_DELETE(basic2D);
	SAFE_DELETE(supportlines);
	SAFE_DELETE(surface);
	SAFE_DELETE(curve);
	SAFE_DELETE(screenquad);

	if( text1 )
		glDeleteTextures(1, &text1);

	text1 = 0;

	GLKillAnyRogueObject();
}
//*************************************************************************************************************
void KeyPress(WPARAM wparam)
{
	switch( wparam )
	{
	case 0x46:
		fullscreen = !fullscreen;
		break;

	case 0x55:
		break;

	case 0x57:
		wireframe = !wireframe;
		break;

	default:
		break;
	}
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
		if( !fullscreen )
			UpdateControlPoints((float)mousex, (float)mousey);

		if( fullscreen ||
			((mousex >= screenwidth - 330 && mousex <= screenwidth - 10) &&
			(mousey >= 10 && mousey <= 260)) )
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
	OpenGLColor	outsidecolor(0.75f, 0.75f, 0.8f, 1);
	OpenGLColor	insidecolor(1, 0.66f, 0.066f, 1);

	float		world[16];
	float		view[16];
	float		proj[16];
	float		viewproj[16];

	float		pointsize[2]	= { 10.0f / screenwidth, 10.0f / screenheight };
	float		grthickness[2]	= { 1.5f / screenwidth, 1.5f / screenheight };
	float		cvthickness[2]	= { 2.0f / screenwidth, 2.0f / screenheight };
	float		spthickness[2]	= { 3.0f / screenwidth, 3.0f / screenheight };
	float		spviewport[]	= { 0, 0, (float)screenwidth, (float)screenheight };

	float		eye[3]			= { 5, 4, 15 };
	float		look[3]			= { 5, 4, 5 };
	float		up[3]			= { 0, 1, 0 };
	float		lightdir[4]		= { 0, 1, 0, 0 };
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
	if( !fullscreen )
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(screenwidth - 330, screenheight - 250, 320, 240);
	}

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);

	glEnable(GL_DEPTH_TEST);

	if( fullscreen )
		glViewport(0, 0, screenwidth, screenheight);
	else
		glViewport(screenwidth - 330, screenheight - 250, 320, 240);

	cameraangle.smooth(orient, alpha);

	GLVec3Subtract(fwd, look, eye);
	GLMatrixRotationYawPitchRoll(view, orient[0], orient[1], 0);
	GLVec3Transform(fwd, fwd, view);
	GLVec3Subtract(eye, look, fwd);

	GLMatrixPerspectiveRH(proj, M_PI / 3, 4.0f / 3.0f, 0.1f, 50.0f);
	GLMatrixLookAtRH(view, eye, look, up);
	GLMatrixMultiply(viewproj, view, proj);

	if( wireframe )
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glDisable(GL_CULL_FACE);

	rendersurface->SetMatrix("matViewProj", viewproj);
	rendersurface->SetMatrix("matWorld", world);
	rendersurface->SetMatrix("matWorldInv", world); // its id
	rendersurface->SetVector("lightDir", lightdir);
	rendersurface->SetVector("eyePos", eye);
	rendersurface->SetVector("outsideColor", &outsidecolor.r);
	rendersurface->SetVector("insideColor", &insidecolor.r);
	rendersurface->SetInt("isWireMode", wireframe);

	rendersurface->Begin();
	{
		surface->DrawSubset(0);
	}
	rendersurface->End();

	glEnable(GL_CULL_FACE);

	if( wireframe )
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// render text
	if( !fullscreen )
	{
		glViewport(3, 0, screenwidth, screenheight - (screenwidth - 330));

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		float xzplane[4] = { 0, 1, 0, -0.5f };
		GLMatrixReflect(world, xzplane);

		basic2D->SetMatrix("matTexture", world);
		basic2D->SetInt("sampler0", 0);
		basic2D->Begin();
		{
			glBindTexture(GL_TEXTURE_2D, text1);
			screenquad->Draw();
		}
		basic2D->End();

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}

#ifdef _DEBUG
	// check errors
	GLenum err = glGetError();

	if( err != GL_NO_ERROR )
		std::cout << "Error\n";
#endif

	SwapBuffers(hdc);
}
//*************************************************************************************************************
