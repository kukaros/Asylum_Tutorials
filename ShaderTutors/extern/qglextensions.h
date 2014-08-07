//=============================================================================================================
#if !defined(_QGLEXTENSIONS_H_)
#define _QGLEXTENSIONS_H_

//#define _Q_TEST_LOW_CONFIG

#ifdef _WIN32
#	define _Q_WINDOWS
#endif

#ifdef __APPLE__
#	define _Q_MAC
#endif

#ifdef _Q_MAC
#	include <OpenGL/gl.h>
#	include <OpenGL/OpenGL.h>
#	include <OpenGL/glext.h>
#elif defined(_Q_WINDOWS)
#	include <Windows.h>
#	include <gl/gl.h>
#	include "../extern/glext.h"
#	include "../extern/glcorearb.h"
#elif defined(_Q_IOS)
#	include <OpenGLES/ES2/gl.h>
#	include <OpenGLES/ES2/glext.h>
#elif defined(_Q_ANDROID)
#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>
#endif

#include <string>

#if defined(_Q_MAC)
#	define GL_SHARING_EXTENSION		"cl_APPLE_gl_sharing"

#	ifndef GL_PROGRAM_POINT_SIZE
#		define GL_PROGRAM_POINT_SIZE	GL_PROGRAM_POINT_SIZE_EXT
#	endif
#else
#	define GL_SHARING_EXTENSION		"cl_khr_gl_sharing"
#endif

typedef std::string qstring;
typedef unsigned short quint16;
typedef unsigned int quint32;

#define qdebugbreak()	throw 1;
#define Q_SSCANF		sscanf

#if defined(_Q_MOBILE) && _Q_GLES == 2
#	undef GL_RGB5_A1	// doesnt work

// workaround
#	define GL_DEPTH_COMPONENT24					GL_DEPTH_COMPONENT16
#	define GL_DEPTH_COMPONENT32					GL_DEPTH_COMPONENT16
#	define GL_DEPTH24_STENCIL8					GL_DEPTH24_STENCIL8_OES
#	define GL_DEPTH_STENCIL_ATTACHMENT			GL_DEPTH_ATTACHMENT
#	define GL_LUMINANCE8						GL_LUMINANCE
#	define GL_LUMINANCE8_ALPHA8					GL_LUMINANCE_ALPHA
#	define GL_CLAMP								GL_CLAMP_TO_EDGE
#	define GL_CLAMP_TO_BORDER					GL_CLAMP_TO_EDGE
#	define GL_RGBA8								GL_RGBA
#	define GL_RGB5_A1							GL_RGBA

// will generate error when used
#	define GL_R32F								0
#	define GL_R16F								0
#	define GL_RG32F								0
#	define GL_RG16F								0
#	define GL_RGB16F_ARB						0
#	define GL_RGBA16F_ARB						0
#	define GL_RGB32F_ARB						0
#	define GL_RGBA32F_ARB						0
#	define GL_SRGB8_ALPHA8						0
#	define GL_RED								0
#	define GL_RG								0
#	define GL_HALF_FLOAT						0
#	define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT		0
#	define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT		0

#	define GL_MODULATE							0
#	define GL_PREVIOUS							0
#	define GL_PRIMARY_COLOR						0
#	define GL_CONSTANT							0
#	define GL_MODELVIEW							0
#	define GL_PROJECTION						0
#	define GL_COLOR_ATTACHMENT1					0
#	define GL_COLOR_ATTACHMENT2					0
#	define GL_COLOR_ATTACHMENT3					0
#	define GL_SMOOTH							0

// unsupported functions
#	define glGetBufferSubData(a, b, c, d)
#	define glDrawBuffers(a, b)
#	define glDrawBuffer(a)

// other
#	define glClearDepth							glClearDepthf
#	define glRenderbufferStorageMultisample		glRenderbufferStorageMultisampleAPPLE
#endif

#define FRAMEBUFFER_SRGB_EXT					0x8DB9
#define FRAMEBUFFER_SRGB_CAPABLE_EXT			0x8DBA
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT		0x20A9
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT		0x20B2
#define CUBEMAP_RENDERTARGET_MASK				0x100

#ifndef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
#	define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS	0x8CD9
#endif

#define MAKE_VERSION(major, minor) \
	((0xff00 & (major << 8)) | minor)

#ifdef _Q_WINDOWS
extern PFNGLACTIVETEXTUREARBPROC					glActiveTexture;
extern PFNGLCLIENTACTIVETEXTUREARBPROC				glClientActiveTexture;
extern PFNGLDELETEBUFFERSARBPROC					glDeleteBuffers;
extern PFNGLBINDBUFFERARBPROC						glBindBuffer;
extern PFNGLGENBUFFERSARBPROC						glGenBuffers;
extern PFNGLBUFFERDATAARBPROC						glBufferData;
extern PFNGLBUFFERSUBDATAARBPROC					glBufferSubData;

extern PFNGLCREATEPROGRAMOBJECTARBPROC				glCreateProgram;
extern PFNGLCREATESHADEROBJECTARBPROC				glCreateShader;
extern PFNGLSHADERSOURCEARBPROC						glShaderSource;
extern PFNGLCOMPILESHADERARBPROC					glCompileShader;
extern PFNGLATTACHOBJECTARBPROC						glAttachShader;
extern PFNGLDETACHOBJECTARBPROC						glDetachShader;
extern PFNGLLINKPROGRAMARBPROC						glLinkProgram;
extern PFNGLGETINFOLOGARBPROC						glGetShaderInfoLog;
extern PFNGLGETINFOLOGARBPROC						glGetProgramInfoLog;
extern PFNGLGETUNIFORMLOCATIONARBPROC				glGetUniformLocation;

extern PFNGLUNIFORMMATRIX2FVARBPROC					glUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX3FVARBPROC					glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVARBPROC					glUniformMatrix4fv;

extern PFNGLUNIFORM1IARBPROC						glUniform1i;
extern PFNGLUNIFORM1FARBPROC						glUniform1f;
extern PFNGLUNIFORM2FARBPROC						glUniform2f;
extern PFNGLUNIFORM3FARBPROC						glUniform3f;
extern PFNGLUNIFORM4FARBPROC						glUniform4f;
extern PFNGLUNIFORM1FVARBPROC						glUniform1fv;
extern PFNGLUNIFORM2FVARBPROC						glUniform2fv;
extern PFNGLUNIFORM3FVARBPROC						glUniform3fv;
extern PFNGLUNIFORM4FVARBPROC						glUniform4fv;

extern PFNGLUSEPROGRAMOBJECTARBPROC					glUseProgram;
extern PFNGLGETPROGRAMIVPROC						glGetProgramiv;
extern PFNGLGETSHADERIVPROC							glGetShaderiv;
extern PFNGLDELETEPROGRAMPROC						glDeleteProgram;
extern PFNGLDELETESHADERPROC						glDeleteShader;

extern PFNGLGETACTIVEUNIFORMARBPROC					glGetActiveUniform;
extern PFNGLGETACTIVEATTRIBPROC						glGetActiveAttrib;
extern PFNGLGETATTRIBLOCATIONPROC					glGetAttribLocation;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC			glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC			glDisableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERARBPROC				glVertexAttribPointer;
extern PFNGLBINDATTRIBLOCATIONARBPROC				glBindAttribLocation;
extern PFNGLBINDFRAGDATALOCATIONPROC				glBindFragDataLocation;

extern PFNGLGENFRAMEBUFFERSEXTPROC					glGenFramebuffers;
extern PFNGLGENRENDERBUFFERSEXTPROC					glGenRenderbuffers;
extern PFNGLBINDFRAMEBUFFEREXTPROC					glBindFramebuffer;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC				glFramebufferTexture2D;
extern PFNGLBINDRENDERBUFFEREXTPROC					glBindRenderbuffer;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC				glRenderbufferStorage;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC			glFramebufferRenderbuffer;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC			glCheckFramebufferStatus;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC				glDeleteFramebuffers;
extern PFNGLDELETERENDERBUFFERSEXTPROC				glDeleteRenderbuffers;
extern PFNGLGENERATEMIPMAPEXTPROC					glGenerateMipmap;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC	glRenderbufferStorageMultisample;
extern PFNGLBLITFRAMEBUFFEREXTPROC					glBlitFramebuffer;

extern PFNGLGETBUFFERSUBDATAARBPROC					glGetBufferSubData;
extern PFNGLMAPBUFFERARBPROC						glMapBuffer;
extern PFNGLMAPBUFFERRANGEPROC						glMapBufferRange;
extern PFNGLUNMAPBUFFERARBPROC						glUnmapBuffer;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC				glCompressedTexImage2D;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC				glCompressedTexImage1D;
extern PFNGLDRAWBUFFERSARBPROC						glDrawBuffers;
extern PFNGLDRAWRANGEELEMENTSPROC					glDrawRangeElements;

// 3.2
extern PFNGLGENVERTEXARRAYSPROC						glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC						glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC					glDeleteVertexArrays;
extern PFNGLPATCHPARAMETERIPROC						glPatchParameteri;
extern PFNGLPATCHPARAMETERFVPROC					glPatchParameterfv;

// 4.3
extern PFNGLDISPATCHCOMPUTEPROC						glDispatchCompute;
extern PFNGLDISPATCHCOMPUTEINDIRECTPROC				glDispatchComputeIndirect;
extern PFNGLBINDIMAGETEXTUREPROC					glBindImageTexture;
extern PFNGLBINDBUFFERBASEPROC						glBindBufferBase;

extern PFNGLGETINTEGERI_VPROC						glGetIntegeri_v;
extern PFNGLDEBUGMESSAGECONTROLPROC					glDebugMessageControl;
extern PFNGLDEBUGMESSAGECALLBACKPROC				glDebugMessageCallback;
extern PFNGLGETDEBUGMESSAGELOGPROC					glGetDebugMessageLog;

// WGL specific
typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALFARPROC)(int);
typedef const char* (APIENTRY *WGLGETEXTENSIONSSTRINGARBPROC)(HDC hdc);
typedef HGLRC (APIENTRY *WGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL (APIENTRY *WGLGETPIXELFORMATATTRIBIVARBPROC)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
typedef BOOL (APIENTRY *WGLGETPIXELFORMATATTRIBFVARBPROC)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
typedef BOOL (APIENTRY *WGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef const GLubyte* (APIENTRY *GLGETSTRINGIPROC)(GLenum  name, GLuint index);

extern PFNWGLSWAPINTERVALFARPROC wglSwapInterval;
extern WGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribs;
extern WGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsString;
extern WGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribiv;
extern WGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfv;
extern WGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormat;
extern GLGETSTRINGIPROC glGetStringi;

#endif

#ifdef _Q_MAC
extern "C" {
	extern void glBindVertexArray(GLuint array);
	extern void glDeleteVertexArrays(GLsizei n, const GLuint *arrays);
	extern void glGenVertexArrays(GLsizei n, GLuint *arrays);
	extern void glBindFragDataLocation(GLuint program, GLuint color, const GLchar *name);
}
#endif

namespace Quadron
{
	class qGLExtensions
	{
	private:
		qGLExtensions();

	public:
		enum glversion
		{
			GL_1_1 = MAKE_VERSION(1, 1),
			GL_1_2 = MAKE_VERSION(1, 2),
			GL_1_3 = MAKE_VERSION(1, 3),
			GL_1_4 = MAKE_VERSION(1, 4),
			GL_1_5 = MAKE_VERSION(1, 5),
			GL_2_0 = MAKE_VERSION(2, 0),
			GL_2_1 = MAKE_VERSION(2, 1),
			GL_3_0 = MAKE_VERSION(3, 0),
			GL_3_1 = MAKE_VERSION(3, 1),
			GL_3_2 = MAKE_VERSION(3, 2),
			GL_3_3 = MAKE_VERSION(3, 3),
			GL_4_0 = MAKE_VERSION(4, 0),
			GL_4_1 = MAKE_VERSION(4, 1),
			GL_4_2 = MAKE_VERSION(4, 2),
			GL_4_3 = MAKE_VERSION(4, 3),
			GL_4_4 = MAKE_VERSION(4, 4)
		};

		enum glslversion
		{
			GLSL_110 = MAKE_VERSION(1, 10),
			GLSL_120 = MAKE_VERSION(1, 20),
			GLSL_130 = MAKE_VERSION(1, 30),
			GLSL_140 = MAKE_VERSION(1, 40),
			GLSL_150 = MAKE_VERSION(1, 50),
			GLSL_330 = MAKE_VERSION(3, 30),
			GLSL_400 = MAKE_VERSION(4, 0),
			GLSL_410 = MAKE_VERSION(4, 10),
			GLSL_420 = MAKE_VERSION(4, 20),
			GLSL_430 = MAKE_VERSION(4, 30),
			GLSL_440 = MAKE_VERSION(4, 40)
		};

		static quint16 GLVersion;
		static quint16 GLSLVersion;

		static bool IsSupported(const char* name);
		static void QueryFeatures(void* dc = 0);

		static bool ARB_vertex_buffer_object;
		static bool ARB_vertex_program;
		static bool ARB_fragment_program;
		static bool ARB_shader_objects;
		static bool ARB_texture_float;
		static bool ARB_texture_non_power_of_two;
		static bool ARB_texture_rg;
		static bool ARB_texture_compression;
		static bool ARB_draw_buffers;
		static bool ARB_vertex_array_object;
		static bool ARB_geometry_shader4;
		static bool ARB_tessellation_shader;
		static bool ARB_compute_shader;
		static bool ARB_shader_image_load_store;
		static bool ARB_shader_storage_buffer_object;
		static bool ARB_shader_atomic_counters;
		static bool ARB_debug_output;

		static bool EXT_texture_compression_s3tc;
		static bool EXT_texture_cube_map;
		static bool EXT_framebuffer_object;
		static bool EXT_framebuffer_sRGB;
		static bool EXT_texture_sRGB;
		static bool EXT_framebuffer_multisample;
		static bool EXT_framebuffer_blit;
		static bool EXT_packed_depth_stencil;

		// GLES
		static bool IMG_texture_compression_pvrtc;
		static bool IMG_user_clip_plane;

#ifdef _Q_WINDOWS
		static bool WGL_EXT_swap_control;
		static bool WGL_ARB_pixel_format;
		static bool WGL_ARB_create_context;
		static bool WGL_ARB_create_context_profile;
#endif
	};
}

#endif
//=============================================================================================================
