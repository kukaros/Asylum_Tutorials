
#ifndef _GLEXT_H_
#define _GLEXT_H_

#include "../extern/qglextensions.h"
#include "../common/orderedarray.hpp"

// NOTE: don't freak out. It was easier to copy-paste...

enum OpenGLDeclType
{
	GLDECLTYPE_FLOAT1 =  0,
	GLDECLTYPE_FLOAT2 =  1,
	GLDECLTYPE_FLOAT3 =  2,
	GLDECLTYPE_FLOAT4 =  3,
	GLDECLTYPE_GLCOLOR =  4
};

enum OpenGLDeclUsage
{
	GLDECLUSAGE_POSITION = 0,
	GLDECLUSAGE_BLENDWEIGHT,
	GLDECLUSAGE_BLENDINDICES,
	GLDECLUSAGE_NORMAL,
	GLDECLUSAGE_PSIZE,
	GLDECLUSAGE_TEXCOORD,
	GLDECLUSAGE_TANGENT,
	GLDECLUSAGE_BINORMAL,
	GLDECLUSAGE_TESSFACTOR,
	GLDECLUSAGE_POSITIONT,
	GLDECLUSAGE_COLOR,
	GLDECLUSAGE_FOG,
	GLDECLUSAGE_DEPTH,
	GLDECLUSAGE_SAMPLE
};

struct OpenGLColor
{
	float r, g, b, a;

	OpenGLColor()
		: r(0), g(0), b(0), a(1)
	{
	}

	OpenGLColor(float _r, float _g, float _b, float _a)
		: r(_r), g(_g), b(_b), a(_a)
	{
	}
};

struct OpenGLVertexElement
{
	GLushort	Stream;
	GLushort	Offset;
	GLubyte		Type;			// OpenGLDeclType
	GLubyte		Usage;			// OpenGLDeclUsage
	GLubyte		UsageIndex;
};

struct OpenGLVertexDeclaration
{
	GLuint stride;
};

struct OpenGLAttributeRange
{
	GLuint		AttribId;
	GLuint		FaceStart;
	GLuint		FaceCount;
	GLuint		VertexStart;
	GLuint		VertexCount;
};

struct OpenGLMaterial
{
	OpenGLColor	Diffuse;
	OpenGLColor	Ambient;
	OpenGLColor	Specular;
	OpenGLColor	Emissive;
	float		Power;
	char*		TextureFile;
};

/**
 * \brief Similar to ID3DXMesh. Core profile only.
 */
class OpenGLMesh
{
	friend bool GLCreateMesh(GLuint, GLuint, GLuint, OpenGLVertexElement*, OpenGLMesh**);

	struct locked_data
	{
		void* ptr;
		GLuint flags;
	};

private:
	OpenGLAttributeRange* subsettable;
	OpenGLVertexDeclaration vertexdecl;
	GLuint numvertices;
	GLuint numindices;
	GLuint vertexbuffer;
	GLuint indexbuffer;
	GLuint vertexlayout;

	locked_data vertexdata_locked;
	locked_data indexdata_locked;

	OpenGLMesh();

	void Destroy();

public:
	~OpenGLMesh();

	bool LockVertexBuffer(GLuint flags, void** data);
	bool LockIndexBuffer(GLuint flags, void** data);

	void DrawSubset(GLuint subset);
	void UnlockVertexBuffer();
	void UnlockIndexBuffer();
	void SetAttributeTable(const OpenGLAttributeRange* table, GLuint size);
};

/**
 * \brief Similar to ID3DXEffect. One technique, core profile only.
 */
class OpenGLEffect
{
	friend bool GLCreateEffectFromFile(const char*, const char*, OpenGLEffect**);
	friend bool GLCreateComputeProgramFromFile(const char*, OpenGLEffect**);

	struct Uniform
	{
		char	Name[32];
		GLint	StartRegister;
		GLint	RegisterCount;
		GLint	Location;
		GLenum	Type;

		mutable bool Changed;

		inline bool operator <(const Uniform& other) const {
			return (0 > strcmp(Name, other.Name));
		}
	};

	typedef mystl::orderedarray<Uniform> uniformtable;

private:
	uniformtable	uniforms;
	GLuint			program;
	float*			floatvalues;
	int*			intvalues;
	unsigned int	floatcap;
	unsigned int	floatsize;
	unsigned int	intcap;
	unsigned int	intsize;

	OpenGLEffect();

	void AddUniform(const char* name, GLuint location, GLuint count, GLenum type);
	void BindAttributes();
	void Destroy();
	void QueryUniforms();

public:
	~OpenGLEffect();

	void Begin();
	void CommitChanges();
	void End();
	void SetMatrix(const char* name, float* value);
	void SetVector(const char* name, float* value);
	void SetFloat(const char* name, float value);
	void SetInt(const char* name, int value);
};

// content functions
bool GLCreateMesh(GLuint numfaces, GLuint numvertices, GLuint options, OpenGLVertexElement* decl, OpenGLMesh** mesh);
bool GLLoadMeshFromQM(const char* file, OpenGLMaterial** materials, GLuint* nummaterials, OpenGLMesh** mesh);
bool GLCreateEffectFromFile(const char* vsfile, const char* psfile, OpenGLEffect** effect);
bool GLCreateComputeProgramFromFile(const char* csfile, OpenGLEffect** effect);

// math functions
float GLVec3Dot(float a[3], float b[3]);
float GLVec3Length(float a[3]);

void GLVec3Normalize(float a[3]);
void GLVec3Cross(float out[3], float a[3], float b[3]);
void GLMatrixLookAtRH(float out[16], float eye[3], float look[3], float up[3]);
void GLMatrixPerspectiveRH(float out[16], float fovy, float aspect, float nearplane, float farplane);
void GLMatrixMultiply(float out[16], float a[16], float b[16]);
void GLMatrixRotationAxis(float out[16], float angle, float x, float y, float z);
void GLMatrixIdentity(float out[16]);

#endif
