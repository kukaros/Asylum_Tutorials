//=============================================================================================================
#include "qglextensions.h"

#if 1

#ifdef _MSC_VER
#	pragma warning (disable:4996)
#endif

#include <cstring>

#define GET_ADDRESS(var, type, name)	\
	if( coreprofile ) \
		var = (type)wglGetProcAddress(#var); \
	else \
		var = (type)wglGetProcAddress(name); \
	if( !var ) qdebugbreak();
// end

#ifdef _Q_WINDOWS
PFNGLACTIVETEXTUREARBPROC				glActiveTexture = 0;
PFNGLCLIENTACTIVETEXTUREARBPROC			glClientActiveTexture = 0;
PFNGLDELETEBUFFERSARBPROC				glDeleteBuffers = 0;
PFNGLBINDBUFFERARBPROC					glBindBuffer = 0;
PFNGLGENBUFFERSARBPROC					glGenBuffers = 0;
PFNGLBUFFERDATAARBPROC					glBufferData = 0;
PFNGLBUFFERSUBDATAARBPROC				glBufferSubData = 0;

PFNGLCREATEPROGRAMOBJECTARBPROC			glCreateProgram = 0;
PFNGLCREATESHADEROBJECTARBPROC			glCreateShader = 0;
PFNGLSHADERSOURCEARBPROC				glShaderSource = 0;
PFNGLCOMPILESHADERARBPROC				glCompileShader = 0;
PFNGLATTACHOBJECTARBPROC				glAttachShader = 0;
PFNGLDETACHOBJECTARBPROC				glDetachShader = 0;
PFNGLLINKPROGRAMARBPROC					glLinkProgram = 0;
PFNGLGETINFOLOGARBPROC					glGetShaderInfoLog = 0;
PFNGLGETINFOLOGARBPROC					glGetProgramInfoLog = 0;
PFNGLGETUNIFORMLOCATIONARBPROC			glGetUniformLocation = 0;

PFNGLUNIFORMMATRIX2FVARBPROC			glUniformMatrix2fv = 0;
PFNGLUNIFORMMATRIX3FVARBPROC			glUniformMatrix3fv = 0;
PFNGLUNIFORMMATRIX4FVARBPROC			glUniformMatrix4fv = 0;

PFNGLUNIFORM1IARBPROC					glUniform1i = 0;
PFNGLUNIFORM1FARBPROC					glUniform1f = 0;
PFNGLUNIFORM2FARBPROC					glUniform2f = 0;
PFNGLUNIFORM3FARBPROC					glUniform3f = 0;
PFNGLUNIFORM4FARBPROC					glUniform4f = 0;
PFNGLUNIFORM1FVARBPROC					glUniform1fv = 0;
PFNGLUNIFORM2FVARBPROC					glUniform2fv = 0;
PFNGLUNIFORM3FVARBPROC					glUniform3fv = 0;
PFNGLUNIFORM4FVARBPROC					glUniform4fv = 0;

PFNGLGETPROGRAMIVPROC					glGetProgramiv = 0;
PFNGLGETSHADERIVPROC					glGetShaderiv = 0;
PFNGLDELETEPROGRAMPROC					glDeleteProgram = 0;
PFNGLDELETESHADERPROC					glDeleteShader = 0;

PFNGLUSEPROGRAMOBJECTARBPROC			glUseProgram = 0;
PFNGLGETACTIVEUNIFORMARBPROC			glGetActiveUniform = 0;
PFNGLGETACTIVEATTRIBPROC				glGetActiveAttrib = 0;
PFNGLGETATTRIBLOCATIONPROC				glGetAttribLocation = 0;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC		glEnableVertexAttribArray = 0;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC	glDisableVertexAttribArray = 0;
PFNGLVERTEXATTRIBPOINTERARBPROC			glVertexAttribPointer = 0;
PFNGLBINDATTRIBLOCATIONARBPROC			glBindAttribLocation = 0;
PFNGLBINDFRAGDATALOCATIONPROC			glBindFragDataLocation = 0;

PFNGLGENFRAMEBUFFERSEXTPROC				glGenFramebuffers = 0;
PFNGLGENRENDERBUFFERSEXTPROC			glGenRenderbuffers = 0;
PFNGLBINDFRAMEBUFFEREXTPROC				glBindFramebuffer = 0;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC		glFramebufferTexture2D = 0;
PFNGLBINDRENDERBUFFEREXTPROC			glBindRenderbuffer = 0;
PFNGLRENDERBUFFERSTORAGEEXTPROC			glRenderbufferStorage = 0;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC		glFramebufferRenderbuffer = 0;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC		glCheckFramebufferStatus = 0;
PFNGLDELETEFRAMEBUFFERSEXTPROC			glDeleteFramebuffers = 0;
PFNGLDELETERENDERBUFFERSEXTPROC			glDeleteRenderbuffers = 0;
PFNGLGENERATEMIPMAPEXTPROC				glGenerateMipmap = 0;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC	glRenderbufferStorageMultisample = 0;
PFNGLBLITFRAMEBUFFEREXTPROC				glBlitFramebuffer = 0;

PFNGLGETBUFFERSUBDATAARBPROC			glGetBufferSubData = 0;
PFNGLMAPBUFFERARBPROC					glMapBuffer = 0;
PFNGLUNMAPBUFFERARBPROC					glUnmapBuffer = 0;
PFNGLCOMPRESSEDTEXIMAGE2DPROC			glCompressedTexImage2D = 0;
PFNGLCOMPRESSEDTEXIMAGE1DPROC			glCompressedTexImage1D = 0;
PFNGLDRAWBUFFERSARBPROC					glDrawBuffers = 0;
PFNGLDRAWRANGEELEMENTSPROC				glDrawRangeElements = 0;

PFNGLGENVERTEXARRAYSPROC				glGenVertexArrays = 0;
PFNGLBINDVERTEXARRAYPROC				glBindVertexArray = 0;
PFNGLDELETEVERTEXARRAYSPROC				glDeleteVertexArrays = 0;

PFNGLDISPATCHCOMPUTEPROC				glDispatchCompute = 0;
PFNGLDISPATCHCOMPUTEINDIRECTPROC		glDispatchComputeIndirect = 0;
PFNGLBINDIMAGETEXTUREPROC				glBindImageTexture = 0;
PFNGLBINDBUFFERBASEPROC					glBindBufferBase = 0;

PFNWGLSWAPINTERVALFARPROC				wglSwapInterval = 0;
WGLCREATECONTEXTATTRIBSARBPROC			wglCreateContextAttribs = 0;
WGLGETEXTENSIONSSTRINGARBPROC			wglGetExtensionsString = 0;
WGLGETPIXELFORMATATTRIBIVARBPROC		wglGetPixelFormatAttribiv = 0;
WGLGETPIXELFORMATATTRIBFVARBPROC		wglGetPixelFormatAttribfv = 0;
WGLCHOOSEPIXELFORMATARBPROC				wglChoosePixelFormat = 0;
GLGETSTRINGIPROC						glGetStringi = 0;
#endif

namespace Quadron
{
	quint16 qGLExtensions::GLSLVersion = 0;
	quint16 qGLExtensions::GLVersion = 0;

	bool qGLExtensions::ARB_vertex_buffer_object = false;
	bool qGLExtensions::ARB_vertex_program = false;
	bool qGLExtensions::ARB_fragment_program = false;
	bool qGLExtensions::ARB_shader_objects = false;
	bool qGLExtensions::ARB_texture_float = false;
	bool qGLExtensions::ARB_texture_non_power_of_two = false;
	bool qGLExtensions::ARB_texture_rg = false;
	bool qGLExtensions::ARB_texture_compression = false;
	bool qGLExtensions::ARB_draw_buffers = false;
	bool qGLExtensions::ARB_vertex_array_object = false;
	bool qGLExtensions::ARB_compute_shader = false;
	bool qGLExtensions::ARB_shader_image_load_store = false;
	bool qGLExtensions::ARB_shader_storage_buffer_object = false;

	bool qGLExtensions::EXT_texture_compression_s3tc = false;
	bool qGLExtensions::EXT_texture_cube_map = false;
	bool qGLExtensions::EXT_framebuffer_object = false;
	bool qGLExtensions::EXT_framebuffer_sRGB = false;
	bool qGLExtensions::EXT_texture_sRGB = false;
	bool qGLExtensions::EXT_framebuffer_multisample = false;
	bool qGLExtensions::EXT_framebuffer_blit = false;
	bool qGLExtensions::EXT_packed_depth_stencil = false;

	bool qGLExtensions::IMG_texture_compression_pvrtc = false;
	bool qGLExtensions::IMG_user_clip_plane = false;

#ifdef _Q_WINDOWS
	bool qGLExtensions::WGL_EXT_swap_control = false;
	bool qGLExtensions::WGL_ARB_pixel_format = false;
	bool qGLExtensions::WGL_ARB_create_context = false;
	bool qGLExtensions::WGL_ARB_create_context_profile = false;

	bool wIsSupported(const char* name, HDC hdc);
#endif

	void qGLExtensions::QueryFeatures(void* dc)
	{
		if( GLVersion > 0 )
			return;

		const char* glversion = (const char*)glGetString(GL_VERSION);
		int major, minor;
		bool isgles = false;

		if( 0 == Q_SSCANF(glversion, "%1d.%2d %*s", &major, &minor) )
		{
			Q_SSCANF(glversion, "OpenGL ES %1d %*s", &major);

			minor = 0;
			isgles = true;
		}

		GLVersion = MAKE_VERSION(major, minor);

#ifdef _Q_WINDOWS
		bool coreprofile = (GLVersion >= GL_3_2);

		if( coreprofile )
			glGetStringi = (GLGETSTRINGIPROC)wglGetProcAddress("glGetStringi");

		wglGetExtensionsString = (WGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");

		if( wglGetExtensionsString && dc )
		{
			HDC hdc = (HDC)dc;

			WGL_EXT_swap_control			= wIsSupported("WGL_EXT_swap_control", hdc);
			WGL_ARB_pixel_format			= wIsSupported("WGL_ARB_pixel_format", hdc);
			WGL_ARB_create_context			= wIsSupported("WGL_ARB_create_context", hdc);
			WGL_ARB_create_context_profile	= wIsSupported("WGL_ARB_create_context_profile", hdc);
		}

		if( WGL_ARB_pixel_format )
		{
			wglGetPixelFormatAttribiv		= (WGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
			wglGetPixelFormatAttribfv		= (WGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribfvARB");
			wglChoosePixelFormat			= (WGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
		}

		if( WGL_ARB_create_context && WGL_ARB_create_context_profile )
			wglCreateContextAttribs			= (WGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

		if( WGL_EXT_swap_control )
			wglSwapInterval = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");
#endif
		
#ifndef _Q_TEST_LOW_CONFIG
		if( isgles )
		{
			ARB_vertex_buffer_object		= true;
			ARB_vertex_program				= true;
			ARB_fragment_program			= true;
			ARB_shader_objects				= true;
			
			EXT_framebuffer_object			= true;
			EXT_texture_cube_map			= true;

			IMG_texture_compression_pvrtc	= IsSupported("GL_IMG_texture_compression_pvrtc");
			IMG_user_clip_plane				= IsSupported("GL_IMG_user_clip_plane");
		}
		else if( coreprofile )
		{
			ARB_vertex_buffer_object		= true;
			ARB_vertex_program				= true;
			ARB_fragment_program			= true;
			ARB_shader_objects				= true;
			ARB_texture_float				= true;
			ARB_texture_non_power_of_two	= true;
			ARB_texture_rg					= true;
			ARB_texture_compression			= true;
			ARB_draw_buffers				= true;
			ARB_vertex_array_object			= true;
			
			EXT_framebuffer_object			= true;
			EXT_texture_cube_map			= true;
			EXT_framebuffer_sRGB			= true;
			EXT_texture_sRGB				= true;
			EXT_texture_compression_s3tc	= true;
			EXT_framebuffer_multisample		= true;
			EXT_framebuffer_blit			= true;
			EXT_packed_depth_stencil		= true;

			ARB_compute_shader					= IsSupported("GL_ARB_compute_shader");
			ARB_shader_image_load_store			= IsSupported("GL_ARB_shader_image_load_store");
			ARB_shader_storage_buffer_object	= IsSupported("GL_ARB_shader_storage_buffer_object");
		}
		else
		{
			ARB_vertex_buffer_object		= IsSupported("GL_ARB_vertex_buffer_object");
			ARB_vertex_program				= IsSupported("GL_ARB_vertex_program");
			ARB_fragment_program			= IsSupported("GL_ARB_fragment_program");
			ARB_shader_objects				= IsSupported("GL_ARB_shader_objects");
			ARB_texture_float				= IsSupported("GL_ARB_texture_float");
			ARB_texture_non_power_of_two	= IsSupported("GL_ARB_texture_non_power_of_two");
			ARB_texture_rg					= IsSupported("GL_ARB_texture_rg");
			ARB_texture_compression			= IsSupported("GL_ARB_texture_compression");
			ARB_draw_buffers				= IsSupported("GL_ARB_draw_buffers");
			ARB_vertex_array_object			= IsSupported("GL_ARB_vertex_array_object");

			EXT_framebuffer_object			= IsSupported("GL_EXT_framebuffer_object");
			EXT_framebuffer_sRGB			= IsSupported("GL_EXT_framebuffer_sRGB");
			EXT_framebuffer_multisample		= IsSupported("GL_EXT_framebuffer_multisample");
			EXT_framebuffer_blit			= IsSupported("GL_EXT_framebuffer_blit");
			EXT_texture_sRGB				= IsSupported("GL_EXT_texture_sRGB");
			EXT_texture_compression_s3tc	= IsSupported("GL_EXT_texture_compression_s3tc");
			EXT_texture_cube_map			= IsSupported("GL_EXT_texture_cube_map");
			EXT_packed_depth_stencil		= IsSupported("GL_EXT_packed_depth_stencil");

			if( !EXT_framebuffer_sRGB )
				EXT_framebuffer_sRGB		= IsSupported("GL_ARB_framebuffer_sRGB");

			if( !EXT_texture_cube_map )
				EXT_texture_cube_map		= IsSupported("GL_ARB_texture_cube_map");
		
			if( !ARB_texture_float )
				ARB_texture_float			= IsSupported("GL_APPLE_float_pixels");
		}
#endif

#if defined(_Q_WINDOWS)
		GET_ADDRESS(glActiveTexture, PFNGLACTIVETEXTUREARBPROC, "glActiveTexture");
		GET_ADDRESS(glClientActiveTexture, PFNGLCLIENTACTIVETEXTUREARBPROC, "glClientActiveTexture");
		GET_ADDRESS(glGenerateMipmap, PFNGLGENERATEMIPMAPEXTPROC, "glGenerateMipmapEXT");
		GET_ADDRESS(glMapBuffer, PFNGLMAPBUFFERARBPROC, "glMapBufferARB");
		GET_ADDRESS(glUnmapBuffer, PFNGLUNMAPBUFFERARBPROC, "glUnmapBufferARB");

		if( ARB_texture_compression )
		{
			GET_ADDRESS(glCompressedTexImage2D, PFNGLCOMPRESSEDTEXIMAGE2DPROC, "glCompressedTexImage2D");
			GET_ADDRESS(glCompressedTexImage1D, PFNGLCOMPRESSEDTEXIMAGE1DPROC, "glCompressedTexImage1D");
		}

		if( ARB_draw_buffers )
			GET_ADDRESS(glDrawBuffers, PFNGLDRAWBUFFERSARBPROC, "glDrawBuffers");

		if( ARB_vertex_array_object )
		{
			GET_ADDRESS(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC, "glGenVertexArrays");
			GET_ADDRESS(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC, "glBindVertexArray");
			GET_ADDRESS(glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC, "glDeleteVertexArrays");
		}

		if( ARB_vertex_buffer_object )
		{
			GET_ADDRESS(glGetBufferSubData, PFNGLGETBUFFERSUBDATAARBPROC, "glGetBufferSubData");
			GET_ADDRESS(glDrawRangeElements, PFNGLDRAWRANGEELEMENTSPROC, "glDrawRangeElements");

			GET_ADDRESS(glDeleteBuffers, PFNGLDELETEBUFFERSARBPROC, "glDeleteBuffersARB");
			GET_ADDRESS(glBindBuffer, PFNGLBINDBUFFERARBPROC, "glBindBufferARB");
			GET_ADDRESS(glGenBuffers, PFNGLGENBUFFERSARBPROC, "glGenBuffersARB");
			GET_ADDRESS(glBufferData, PFNGLBUFFERDATAARBPROC, "glBufferDataARB");
			GET_ADDRESS(glBufferSubData, PFNGLBUFFERSUBDATAARBPROC, "glBufferSubDataARB");
		}

		if( EXT_framebuffer_object )
		{
			GET_ADDRESS(glGenFramebuffers, PFNGLGENFRAMEBUFFERSEXTPROC, "glGenFramebuffersEXT");
			GET_ADDRESS(glGenRenderbuffers, PFNGLGENRENDERBUFFERSEXTPROC, "glGenRenderbuffersEXT");
			GET_ADDRESS(glBindFramebuffer, PFNGLBINDFRAMEBUFFEREXTPROC, "glBindFramebufferEXT");
			GET_ADDRESS(glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DEXTPROC, "glFramebufferTexture2DEXT");
			GET_ADDRESS(glBindRenderbuffer, PFNGLBINDRENDERBUFFEREXTPROC, "glBindRenderbufferEXT");
			GET_ADDRESS(glRenderbufferStorage, PFNGLRENDERBUFFERSTORAGEEXTPROC, "glRenderbufferStorageEXT");
			GET_ADDRESS(glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC, "glFramebufferRenderbufferEXT");
			GET_ADDRESS(glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC, "glCheckFramebufferStatusEXT");
			GET_ADDRESS(glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSEXTPROC, "glDeleteFramebuffersEXT");
			GET_ADDRESS(glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERSEXTPROC, "glDeleteRenderbuffersEXT");
		}

		if( EXT_framebuffer_multisample )
			GET_ADDRESS(glRenderbufferStorageMultisample, PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC, "glRenderbufferStorageMultisampleEXT");

		if( EXT_framebuffer_blit )
			GET_ADDRESS(glBlitFramebuffer, PFNGLBLITFRAMEBUFFEREXTPROC, "glBlitFramebufferEXT");

		if( ARB_shader_objects )
		{
			GET_ADDRESS(glCreateProgram, PFNGLCREATEPROGRAMOBJECTARBPROC, "glCreateProgramObjectARB");
			GET_ADDRESS(glCreateShader, PFNGLCREATESHADEROBJECTARBPROC, "glCreateShaderObjectARB");
			GET_ADDRESS(glShaderSource, PFNGLSHADERSOURCEARBPROC, "glShaderSourceARB");
			GET_ADDRESS(glCompileShader, PFNGLCOMPILESHADERARBPROC, "glCompileShaderARB");
			GET_ADDRESS(glAttachShader, PFNGLATTACHOBJECTARBPROC, "glAttachObjectARB");
			GET_ADDRESS(glDetachShader, PFNGLDETACHOBJECTARBPROC, "glDetachObjectARB");
			GET_ADDRESS(glLinkProgram, PFNGLLINKPROGRAMARBPROC, "glLinkProgramARB");
			GET_ADDRESS(glDeleteProgram, PFNGLDELETEOBJECTARBPROC, "glDeleteObjectARB");
			GET_ADDRESS(glDeleteShader, PFNGLDELETEOBJECTARBPROC, "glDeleteObjectARB");
			GET_ADDRESS(glUseProgram, PFNGLUSEPROGRAMOBJECTARBPROC, "glUseProgramObjectARB");

			GET_ADDRESS(glUniformMatrix2fv, PFNGLUNIFORMMATRIX2FVARBPROC, "glUniformMatrix2fvARB");
			GET_ADDRESS(glUniformMatrix3fv, PFNGLUNIFORMMATRIX3FVARBPROC, "glUniformMatrix3fvARB");
			GET_ADDRESS(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVARBPROC, "glUniformMatrix4fvARB");
			GET_ADDRESS(glUniform1i, PFNGLUNIFORM1IARBPROC, "glUniform1iARB");
			GET_ADDRESS(glUniform1f, PFNGLUNIFORM1FARBPROC, "glUniform1fARB");
			GET_ADDRESS(glUniform2f, PFNGLUNIFORM2FARBPROC, "glUniform2fARB");
			GET_ADDRESS(glUniform3f, PFNGLUNIFORM3FARBPROC, "glUniform3fARB");
			GET_ADDRESS(glUniform4f, PFNGLUNIFORM4FARBPROC, "glUniform4fARB");
			GET_ADDRESS(glUniform1fv, PFNGLUNIFORM1FVARBPROC, "glUniform1fvARB");
			GET_ADDRESS(glUniform2fv, PFNGLUNIFORM2FVARBPROC, "glUniform2fvARB");
			GET_ADDRESS(glUniform3fv, PFNGLUNIFORM3FVARBPROC, "glUniform3fvARB");
			GET_ADDRESS(glUniform4fv, PFNGLUNIFORM4FVARBPROC, "glUniform4fvARB");

			GET_ADDRESS(glGetProgramiv, PFNGLGETOBJECTPARAMETERIVARBPROC, "glGetObjectParameterivARB");
			GET_ADDRESS(glGetShaderiv, PFNGLGETOBJECTPARAMETERIVARBPROC, "glGetObjectParameterivARB");
			GET_ADDRESS(glGetActiveUniform, PFNGLGETACTIVEUNIFORMARBPROC, "glGetActiveUniformARB");
			GET_ADDRESS(glGetActiveAttrib, PFNGLGETACTIVEATTRIBPROC, "glGetActiveAttribARB");
			GET_ADDRESS(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC, "glGetAttribLocationARB");
			GET_ADDRESS(glGetShaderInfoLog, PFNGLGETINFOLOGARBPROC, "glGetInfoLogARB");
			GET_ADDRESS(glGetProgramInfoLog, PFNGLGETINFOLOGARBPROC, "glGetInfoLogARB");
			GET_ADDRESS(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONARBPROC, "glGetUniformLocationARB");

			GET_ADDRESS(glBindAttribLocation, PFNGLBINDATTRIBLOCATIONARBPROC, "glBindAttribLocationARB");
			GET_ADDRESS(glBindFragDataLocation, PFNGLBINDFRAGDATALOCATIONPROC, "glBindFragDataLocation");

			GET_ADDRESS(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYARBPROC, "glEnableVertexAttribArrayARB");
			GET_ADDRESS(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYARBPROC, "glDisableVertexAttribArrayARB");
			GET_ADDRESS(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERARBPROC, "glVertexAttribPointerARB");
		}

		// core profile only
		if( ARB_compute_shader )
		{
			GET_ADDRESS(glDispatchCompute, PFNGLDISPATCHCOMPUTEPROC, "");
			GET_ADDRESS(glDispatchComputeIndirect, PFNGLDISPATCHCOMPUTEINDIRECTPROC, "");
		}

		if( ARB_shader_image_load_store )
		{
			GET_ADDRESS(glBindImageTexture, PFNGLBINDIMAGETEXTUREPROC, "");
		}

		if( ARB_shader_storage_buffer_object )
		{
			GET_ADDRESS(glBindBufferBase, PFNGLBINDBUFFERBASEPROC, "");
		}
#endif
		
		if( ARB_shader_objects )
		{
			const char* glslversion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
			int major, minor;
			
			Q_SSCANF(glslversion, "%1d.%2d %*s", &major, &minor);
			GLSLVersion = MAKE_VERSION(major, minor);
		}
		else
			GLSLVersion = 0;
	}

	bool qGLExtensions::IsSupported(const char* name)
	{
		const char *ext = 0, *start;
		const char *loc, *term;

		loc = strchr(name, ' ');

		if( loc || *name == '\0' )
			return false;
		
#ifdef _Q_WINDOWS
		if( glGetStringi )
		{
			GLint numext = 0;

			glGetIntegerv(GL_NUM_EXTENSIONS, &numext);

			for( GLint i = 0; i < numext; ++i )
			{
				ext = (const char*)glGetStringi(GL_EXTENSIONS, i);

				if( 0 == strcmp(ext, name) )
					return true;
			}
		}
		else
		{
#endif
			ext = (const char*)glGetString(GL_EXTENSIONS);
			start = ext;

			if( !ext )
				return false;

			for( ;; )
			{
				if( !(loc = strstr(start, name)) )
					break;

				term = loc + strlen(name);

				if( loc == start || *(loc - 1) == ' ' )
				{
					if( *term == ' ' || *term == '\0' )
						return true;
				}

				start = term;
			}
#ifdef _Q_WINDOWS
		}
#endif

		return false;
	}

#ifdef _Q_WINDOWS
	bool wIsSupported(const char* name, HDC hdc)
	{
		const char *ext = 0, *start;
		const char *loc, *term;

		loc = strchr(name, ' ');

		if( loc || *name == '\0' )
			return false;

		ext = (const char*)wglGetExtensionsString(hdc);
		start = ext;

		for( ;; )
		{
			if( !(loc = strstr(start, name)) )
				break;

			term = loc + strlen(name);

			if( loc == start || *(loc - 1) == ' ' )
			{
				if( *term == ' ' || *term == '\0' )
					return true;
			}

			start = term;
		}

		return false;
	}
#endif
}

#endif
//=============================================================================================================
