//=============================================================================================================
#if !defined(_QGL2EXTENSIONS_H_)
#define _QGL2EXTENSIONS_H_

#ifdef _Q_MAC
#	include <OpenGL/gl.h>
#else
#	include <Windows.h>
#	include <gl/gl.h>
#endif

#include "glext.h"

extern PFNGLACTIVETEXTUREARBPROC				glActiveTexture;
extern PFNGLCLIENTACTIVETEXTUREARBPROC			glClientActiveTexture;
extern PFNGLDELETEBUFFERSARBPROC				glDeleteBuffers;
extern PFNGLBINDBUFFERARBPROC					glBindBuffer;
extern PFNGLGENBUFFERSARBPROC					glGenBuffers;
extern PFNGLBUFFERDATAARBPROC					glBufferData;
extern PFNGLBUFFERSUBDATAARBPROC				glBufferSubData;

extern PFNGLCREATEPROGRAMOBJECTARBPROC			glCreateProgram;
extern PFNGLCREATESHADEROBJECTARBPROC			glCreateShader;
extern PFNGLSHADERSOURCEARBPROC					glShaderSource;
extern PFNGLCOMPILESHADERARBPROC				glCompileShader;
extern PFNGLATTACHOBJECTARBPROC					glAttachShader;
extern PFNGLLINKPROGRAMARBPROC					glLinkProgram;
extern PFNGLGETINFOLOGARBPROC					glGetShaderInfoLog;
extern PFNGLGETINFOLOGARBPROC					glGetProgramInfoLog;
extern PFNGLGETUNIFORMLOCATIONARBPROC			glGetUniformLocation;

extern PFNGLUNIFORMMATRIX2FVARBPROC				glUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX3FVARBPROC				glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVARBPROC				glUniformMatrix4fv;

extern PFNGLUNIFORM1IARBPROC					glUniform1i;
extern PFNGLUNIFORM1FARBPROC					glUniform1f;
extern PFNGLUNIFORM2FARBPROC					glUniform2f;
extern PFNGLUNIFORM3FARBPROC					glUniform3f;
extern PFNGLUNIFORM4FARBPROC					glUniform4f;
extern PFNGLUNIFORM1FVARBPROC					glUniform1fv;
extern PFNGLUNIFORM2FVARBPROC					glUniform2fv;
extern PFNGLUNIFORM3FVARBPROC					glUniform3fv;
extern PFNGLUNIFORM4FVARBPROC					glUniform4fv;

extern PFNGLDELETEOBJECTARBPROC					glDeleteObject;
extern PFNGLUSEPROGRAMOBJECTARBPROC				glUseProgram;
extern PFNGLGETOBJECTPARAMETERIVARBPROC			glGetObjectParameteriv;
extern PFNGLGETACTIVEUNIFORMARBPROC				glGetActiveUniform;
extern PFNGLGETACTIVEATTRIBPROC					glGetActiveAttrib;
extern PFNGLGETATTRIBLOCATIONPROC				glGetAttribLocation;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC		glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC		glDisableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERARBPROC			glVertexAttribPointer;

extern PFNGLGENFRAMEBUFFERSEXTPROC				glGenFramebuffers;
extern PFNGLGENRENDERBUFFERSEXTPROC				glGenRenderbuffers;
extern PFNGLBINDFRAMEBUFFEREXTPROC				glBindFramebuffer;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC			glFramebufferTexture2D;
extern PFNGLBINDRENDERBUFFEREXTPROC				glBindRenderbuffer;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC			glRenderbufferStorage;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC		glFramebufferRenderbuffer;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC		glCheckFramebufferStatus;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC			glDeleteFramebuffers;
extern PFNGLDELETERENDERBUFFERSEXTPROC			glDeleteRenderbuffers;
extern PFNGLGENERATEMIPMAPEXTPROC				glGenerateMipmap;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC	glRenderbufferStorageMultisample;
extern PFNGLBLITFRAMEBUFFEREXTPROC				glBlitFramebuffer;

extern PFNGLGETBUFFERSUBDATAARBPROC				glGetBufferSubData;
extern PFNGLMAPBUFFERARBPROC					glMapBuffer;
extern PFNGLUNMAPBUFFERARBPROC					glUnmapBuffer;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC			glCompressedTexImage2D;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC			glCompressedTexImage1D;
extern PFNGLDRAWBUFFERSARBPROC					glDrawBuffers;
extern PFNGLDRAWRANGEELEMENTSPROC				glDrawRangeElements;

typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALFARPROC)(int);
extern PFNWGLSWAPINTERVALFARPROC wglSwapInterval;

#define glGetProgramiv							glGetObjectParameteriv
#define glGetShaderiv							glGetObjectParameteriv
#define glDeleteProgram							glDeleteObject
#define glDeleteShader							glDeleteObject

#define FRAMEBUFFER_SRGB_EXT					0x8DB9
#define FRAMEBUFFER_SRGB_CAPABLE_EXT			0x8DBA
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT		0x20A9
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT		0x20B2

#ifndef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
#	define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS	0x8CD9
#endif

#define MAKE_VERSION(major, minor) \
	((0xff00 & (major << 8)) | minor)

namespace Quadron
{
	class qGL2Extensions
	{
	public:
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
			GLSL_430 = MAKE_VERSION(4, 30)
		};

		static unsigned short GLSLVersion;

		static bool IsSupported(const char* name);
		static void QueryFeatures();

		static bool ARB_vertex_buffer_object;
		static bool ARB_vertex_program;
		static bool ARB_fragment_program;
		static bool ARB_shader_objects;
		static bool ARB_texture_float;
		static bool ARB_texture_non_power_of_two;
		static bool ARB_texture_rg;
		static bool ARB_texture_compression;
		static bool ARB_draw_buffers;

		static bool EXT_texture_compression_s3tc;
		static bool EXT_texture_cube_map;
		static bool EXT_framebuffer_object;
		static bool EXT_framebuffer_sRGB;
		static bool EXT_texture_sRGB;
		static bool EXT_framebuffer_multisample;
		static bool EXT_framebuffer_blit;
		static bool EXT_packed_depth_stencil;

		static bool WGL_EXT_swap_control;
	};
}

#endif
//=============================================================================================================
