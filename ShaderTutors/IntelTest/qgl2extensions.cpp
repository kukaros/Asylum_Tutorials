//=============================================================================================================
#include "qgl2extensions.h"
#include <string>

#define qstring std::string

#ifdef _WIN32
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

PFNGLDELETEOBJECTARBPROC				glDeleteObject = 0;
PFNGLUSEPROGRAMOBJECTARBPROC			glUseProgram = 0;
PFNGLGETOBJECTPARAMETERIVARBPROC		glGetObjectParameteriv = 0;
PFNGLGETACTIVEUNIFORMARBPROC			glGetActiveUniform = 0;
PFNGLGETACTIVEATTRIBPROC				glGetActiveAttrib = 0;
PFNGLGETATTRIBLOCATIONPROC				glGetAttribLocation = 0;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC		glEnableVertexAttribArray = 0;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC	glDisableVertexAttribArray = 0;
PFNGLVERTEXATTRIBPOINTERARBPROC			glVertexAttribPointer = 0;

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

PFNWGLSWAPINTERVALFARPROC				wglSwapInterval = 0;
GLGETSTRINGIPROC						glGetStringi = 0;
#endif

namespace Quadron
{
	quint16 qGL2Extensions::GLSLVersion = 0;
	quint16 qGL2Extensions::GLVersion = 0;

	bool qGL2Extensions::ARB_vertex_buffer_object = false;
	bool qGL2Extensions::ARB_vertex_program = false;
	bool qGL2Extensions::ARB_fragment_program = false;
	bool qGL2Extensions::ARB_shader_objects = false;
	bool qGL2Extensions::ARB_texture_float = false;
	bool qGL2Extensions::ARB_texture_non_power_of_two = false;
	bool qGL2Extensions::ARB_texture_rg = false;
	bool qGL2Extensions::ARB_texture_compression = false;
	bool qGL2Extensions::ARB_draw_buffers = false;
	bool qGL2Extensions::ARB_vertex_array_object = false;

	bool qGL2Extensions::EXT_texture_compression_s3tc = false;
	bool qGL2Extensions::EXT_texture_cube_map = false;
	bool qGL2Extensions::EXT_framebuffer_object = false;
	bool qGL2Extensions::EXT_framebuffer_sRGB = false;
	bool qGL2Extensions::EXT_texture_sRGB = false;
	bool qGL2Extensions::EXT_framebuffer_multisample = false;
	bool qGL2Extensions::EXT_framebuffer_blit = false;
	bool qGL2Extensions::EXT_packed_depth_stencil = false;

#ifdef _WIN32
	bool qGL2Extensions::WGL_EXT_swap_control = false;
#endif

	bool qGL2Extensions::IsSupported(const char* name)
	{
		const char *ext = 0, *start;
		const char *loc, *term;

		loc = strchr(name, ' ');

		if( loc || *name == '\0' )
			return false;

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
		}

		return false;
	}

	void qGL2Extensions::QueryFeatures()
	{
		const char* glversion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
		int major, minor;

#ifdef _MSC_VER
		sscanf_s(glversion, "%1d.%2d %*s", &major, &minor);
#else
		sscanf(glversion, "%1d.%2d %*s", &major, &minor);
#endif
		GLVersion = MAKE_VERSION(major, minor);

		if( GLVersion >= GL_3_2 )
			glGetStringi = (GLGETSTRINGIPROC)wglGetProcAddress("glGetStringi");

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
		EXT_texture_cube_map			= IsSupported("GL_EXT_texture_cube_map");
		EXT_framebuffer_sRGB			= IsSupported("GL_EXT_framebuffer_sRGB");
		EXT_texture_sRGB				= IsSupported("GL_EXT_texture_sRGB");
		EXT_texture_compression_s3tc	= IsSupported("GL_EXT_texture_compression_s3tc");
		EXT_framebuffer_multisample		= IsSupported("GL_EXT_framebuffer_multisample");
		EXT_framebuffer_blit			= IsSupported("GL_EXT_framebuffer_blit");
		EXT_packed_depth_stencil		= IsSupported("GL_EXT_packed_depth_stencil");

		if( !EXT_texture_cube_map )
			EXT_texture_cube_map		= IsSupported("GL_ARB_texture_cube_map");

#ifdef __APPLE__
		if( !ARB_texture_float )
			ARB_texture_float			= IsSupported("GL_APPLE_float_pixels");
#endif

#if defined(_WIN32)
		// query opengl2 functions
		glActiveTexture				= (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTexture");
		glClientActiveTexture		= (PFNGLCLIENTACTIVETEXTUREARBPROC)wglGetProcAddress("glClientActiveTexture");
		
		glDeleteBuffers				= (PFNGLDELETEBUFFERSARBPROC)wglGetProcAddress("glDeleteBuffersARB");
		glBindBuffer				= (PFNGLBINDBUFFERARBPROC)wglGetProcAddress("glBindBufferARB");
		glGenBuffers				= (PFNGLGENBUFFERSARBPROC)wglGetProcAddress("glGenBuffersARB");
		glBufferData				= (PFNGLBUFFERDATAARBPROC)wglGetProcAddress("glBufferDataARB");
		glBufferSubData				= (PFNGLBUFFERSUBDATAARBPROC)wglGetProcAddress("glBufferSubDataARB");
		
		glCreateProgram				= (PFNGLCREATEPROGRAMOBJECTARBPROC)wglGetProcAddress("glCreateProgramObjectARB");
		glCreateShader				= (PFNGLCREATESHADEROBJECTARBPROC)wglGetProcAddress("glCreateShaderObjectARB");
		glShaderSource				= (PFNGLSHADERSOURCEARBPROC)wglGetProcAddress("glShaderSourceARB");
		glCompileShader				= (PFNGLCOMPILESHADERARBPROC)wglGetProcAddress("glCompileShaderARB");
		glAttachShader				= (PFNGLATTACHOBJECTARBPROC)wglGetProcAddress("glAttachObjectARB");
		glDetachShader				= (PFNGLDETACHOBJECTARBPROC)wglGetProcAddress("glDetachObjectARB");
		glLinkProgram				= (PFNGLLINKPROGRAMARBPROC)wglGetProcAddress("glLinkProgramARB");
		glGetShaderInfoLog			= (PFNGLGETINFOLOGARBPROC)wglGetProcAddress("glGetInfoLogARB");
		glGetProgramInfoLog			= (PFNGLGETINFOLOGARBPROC)wglGetProcAddress("glGetInfoLogARB");
		glGetUniformLocation		= (PFNGLGETUNIFORMLOCATIONARBPROC)wglGetProcAddress("glGetUniformLocationARB");

		glUniformMatrix2fv			= (PFNGLUNIFORMMATRIX2FVARBPROC)wglGetProcAddress("glUniformMatrix2fvARB");
		glUniformMatrix3fv			= (PFNGLUNIFORMMATRIX3FVARBPROC)wglGetProcAddress("glUniformMatrix3fvARB");
		glUniformMatrix4fv			= (PFNGLUNIFORMMATRIX4FVARBPROC)wglGetProcAddress("glUniformMatrix4fvARB");

		glUniform1i					= (PFNGLUNIFORM1IARBPROC)wglGetProcAddress("glUniform1iARB");
		glUniform1f					= (PFNGLUNIFORM1FARBPROC)wglGetProcAddress("glUniform1fARB");
		glUniform2f					= (PFNGLUNIFORM2FARBPROC)wglGetProcAddress("glUniform2fARB");
		glUniform3f					= (PFNGLUNIFORM3FARBPROC)wglGetProcAddress("glUniform3fARB");
		glUniform4f					= (PFNGLUNIFORM4FARBPROC)wglGetProcAddress("glUniform4fARB");
		glUniform1fv				= (PFNGLUNIFORM1FVARBPROC)wglGetProcAddress("glUniform1fvARB");
		glUniform2fv				= (PFNGLUNIFORM2FVARBPROC)wglGetProcAddress("glUniform2fvARB");
		glUniform3fv				= (PFNGLUNIFORM3FVARBPROC)wglGetProcAddress("glUniform3fvARB");
		glUniform4fv				= (PFNGLUNIFORM4FVARBPROC)wglGetProcAddress("glUniform4fvARB");

		glDeleteObject				= (PFNGLDELETEOBJECTARBPROC)wglGetProcAddress("glDeleteObjectARB");
		glUseProgram				= (PFNGLUSEPROGRAMOBJECTARBPROC)wglGetProcAddress("glUseProgramObjectARB");
		glGetObjectParameteriv		= (PFNGLGETOBJECTPARAMETERIVARBPROC)wglGetProcAddress("glGetObjectParameterivARB");
		glGetActiveUniform			= (PFNGLGETACTIVEUNIFORMARBPROC)wglGetProcAddress("glGetActiveUniformARB");
		glGetActiveAttrib			= (PFNGLGETACTIVEATTRIBPROC)wglGetProcAddress("glGetActiveAttribARB");
		glGetAttribLocation			= (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocationARB");
		glEnableVertexAttribArray	= (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)wglGetProcAddress("glEnableVertexAttribArrayARB");
		glDisableVertexAttribArray	= (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)wglGetProcAddress("glDisableVertexAttribArrayARB");
		glVertexAttribPointer		= (PFNGLVERTEXATTRIBPOINTERARBPROC)wglGetProcAddress("glVertexAttribPointerARB");
		
		glGenFramebuffers			= (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
		glGenRenderbuffers			= (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
		glBindFramebuffer			= (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
		glFramebufferTexture2D		= (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
		glBindRenderbuffer			= (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
		glRenderbufferStorage		= (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
		glFramebufferRenderbuffer	= (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");
		glCheckFramebufferStatus	= (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
		glDeleteFramebuffers		= (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
		glDeleteRenderbuffers		= (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");
		glGenerateMipmap			= (PFNGLGENERATEMIPMAPEXTPROC)wglGetProcAddress("glGenerateMipmapEXT");

		glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)wglGetProcAddress("glRenderbufferStorageMultisampleEXT");
		glBlitFramebuffer			= (PFNGLBLITFRAMEBUFFEREXTPROC)wglGetProcAddress("glBlitFramebufferEXT");

		glGetBufferSubData			= (PFNGLGETBUFFERSUBDATAARBPROC)wglGetProcAddress("glGetBufferSubData");
		glMapBuffer					= (PFNGLMAPBUFFERARBPROC)wglGetProcAddress("glMapBufferARB");
		glUnmapBuffer				= (PFNGLUNMAPBUFFERARBPROC)wglGetProcAddress("glUnmapBufferARB");
		glCompressedTexImage2D		= (PFNGLCOMPRESSEDTEXIMAGE2DPROC)wglGetProcAddress("glCompressedTexImage2D");
		glCompressedTexImage1D		= (PFNGLCOMPRESSEDTEXIMAGE1DPROC)wglGetProcAddress("glCompressedTexImage1D");
		glDrawBuffers				= (PFNGLDRAWBUFFERSARBPROC)wglGetProcAddress("glDrawBuffers");
		glDrawRangeElements			= (PFNGLDRAWRANGEELEMENTSPROC)wglGetProcAddress("glDrawRangeElements");

		glGenVertexArrays			= (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
		glBindVertexArray			= (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
		glDeleteVertexArrays		= (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");

		if( (WGL_EXT_swap_control = IsSupported("WGL_EXT_swap_control")) )
			wglSwapInterval = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");
#endif
		
		if( ARB_shader_objects )
		{
			const char* glslversion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
			
#ifdef _MSC_VER
			sscanf_s(glslversion, "%1d.%2d %*s", &major, &minor);
#else
			sscanf(glslversion, "%1d.%2d %*s", &major, &minor);
#endif
			GLSLVersion = MAKE_VERSION(major, minor);
		}
		else
			GLSLVersion = 0;
	}
}
//=============================================================================================================
