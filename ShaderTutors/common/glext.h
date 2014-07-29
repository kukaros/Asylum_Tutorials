
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

enum OpenGLFormat
{
	GLFMT_UNKNOWN = 0,
	GLFMT_R8G8B8,
	GLFMT_A8R8G8B8,
	GLFMT_sA8R8G8B8,
	GLFMT_D24S8,
	GLFMT_D32F,

	GLFMT_R16F,
	GLFMT_G16R16F,
	GLFMT_A16B16G16R16F,

	GLFMT_R32F,
	GLFMT_G32R32F,
	GLFMT_A32B32G32R32F
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
 * \brief Axis-aligned bounding box
 */
class OpenGLAABox
{
public:
	float Min[3];
	float Max[3];

	OpenGLAABox();

	void Add(float x, float y, float z);
	void Add(float v[3]);
	void GetCenter(float out[3]);
	void GetHalfSize(float out[3]);
	void GetPlanes(float outplanes[6][4]);
	void TransformAxisAligned(float traf[16]);

	float Radius() const;
	float Nearest(float from[4]) const;
	float Farthest(float from[4]) const;
};

/**
 * \brief Similar to ID3DXMesh. Core profile only.
 */
class OpenGLMesh
{
	friend bool GLCreateMesh(GLuint, GLuint, GLuint, OpenGLVertexElement*, OpenGLMesh**);
	friend bool GLLoadMeshFromQM(const char*, OpenGLMaterial**, GLuint*, OpenGLMesh**);

	struct locked_data
	{
		void* ptr;
		GLuint flags;
	};

private:
	OpenGLAttributeRange*		subsettable;
	OpenGLVertexDeclaration		vertexdecl;
	OpenGLAABox					boundingbox;
	GLuint						numvertices;
	GLuint						numindices;
	GLuint						vertexbuffer;
	GLuint						indexbuffer;
	GLuint						vertexlayout;

	locked_data					vertexdata_locked;
	locked_data					indexdata_locked;

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

	inline const OpenGLAABox& GetBoundingBox() const {
		return boundingbox;
	}

	inline GLuint GetVertexBuffer() const {
		return vertexbuffer;
	}

	inline GLuint GetIndexBuffer() const {
		return indexbuffer;
	}
};

/**
 * \brief Similar to ID3DXEffect. One technique, core profile only.
 */
class OpenGLEffect
{
	friend bool GLCreateEffectFromFile(const char*, const char*, OpenGLEffect**);
	friend bool GLCreateComputeProgramFromFile(const char*, const char*, OpenGLEffect**);

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

/**
 * brief FBO with attachments
 */
class OpenGLFramebuffer
{
	struct Attachment
	{
		GLuint id;
		int type;

		Attachment()
			: id(0), type(0) {}
	};

private:
	GLuint		fboid;
	GLuint		sizex;
	GLuint		sizey;
	Attachment	rendertargets[8];
	Attachment	depthstencil;

public:
	OpenGLFramebuffer(GLuint width, GLuint height);
	~OpenGLFramebuffer();

	bool AttachRenderbuffer(GLenum target, OpenGLFormat format);
	bool AttachTexture(GLenum target, OpenGLFormat format);
	bool Validate();

	void Set();
	void Unset();

	inline GLuint GetColorAttachment(int index) {
		return rendertargets[index].id;
	}
	
	inline GLuint GetDepthAttachment() {
		return depthstencil.id;
	}
};

/**
 * Simple quad for 2D rendering
 */
class OpenGLScreenQuad
{
private:
	GLuint vertexbuffer;
	GLuint vertexlayout;

public:
	OpenGLScreenQuad();
	~OpenGLScreenQuad();

	void Draw();
};

// content functions
bool GLCreateMesh(GLuint numfaces, GLuint numvertices, GLuint options, OpenGLVertexElement* decl, OpenGLMesh** mesh);
bool GLLoadMeshFromQM(const char* file, OpenGLMaterial** materials, GLuint* nummaterials, OpenGLMesh** mesh);
bool GLCreateEffectFromFile(const char* vsfile, const char* psfile, OpenGLEffect** effect);
bool GLCreateComputeProgramFromFile(const char* csfile, const char* defines, OpenGLEffect** effect);
bool GLCreateTextureFromFile(const char* file, bool srgb, GLuint* out);

void GLKillAnyRogueObject();

// math functions
int isqrt(int n);

float GLVec3Dot(const float a[3], const float b[3]);
float GLVec3Length(const float a[3]);
float GLVec3Distance(const float a[3], const float b[3]);

void GLVec3Normalize(float out[3], const float v[3]);
void GLVec3Cross(float out[3], const float a[3], const float b[3]);
void GLVec3Transform(float out[3], const float v[3], const float m[16]);
void GLVec3TransformCoord(float out[3], const float v[3], const float m[16]);
void GLVec4Transform(float out[4], const float v[4], const float m[16]);
void GLVec4TransformTranspose(float out[4], const float m[16], const float v[4]);
void GLPlaneNormalize(float out[4], const float p[4]);

void GLMatrixLookAtRH(float out[16], const float eye[3], const float look[3], const float up[3]);
void GLMatrixPerspectiveRH(float out[16], float fovy, float aspect, float nearplane, float farplane);
void GLMatrixMultiply(float out[16], const float a[16], const float b[16]);
void GLMatrixRotationAxis(float out[16], float angle, float x, float y, float z);
void GLMatrixRotationYawPitchRoll(float out[16], float yaw, float pitch, float roll);
void GLMatrixIdentity(float out[16]);
void GLMatrixInverse(float out[16], const float m[16]);
void GLMatrixReflect(float out[16], float plane[4]);

void GLFitToBox(float& outnear, float& outfar, const float eye[3], const float look[3], const OpenGLAABox& box);
void GLGetOrthogonalVectors(float out1[3], float out2[3], const float v[3]);

// other
template <typename T, int n>
struct array_state
{
	T prev[n];
	T curr[n];

	array_state& operator =(T t[n]) {
		for (int i = 0; i < n; ++i)
			prev[i] = curr[i] = t[i];

		return *this;
	}

	void smooth(T out[n], float alpha) {
		for (int i = 0; i < n; ++i)
			out[i] = prev[i] + alpha * (curr[i] - prev[i]);
	}
};

#endif
