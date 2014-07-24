
#include "glext.h"

#include <iostream>
#include <map>
#include <string>
#include <cstdio>
#include <cmath>

#ifdef _MSC_VER
#	pragma warning (disable:4996)
#endif

GLint map_Format_Internal[12] =
{
	0,
	GL_RGB8,
	GL_RGBA8,
	GL_SRGB8_ALPHA8,
	GL_DEPTH_STENCIL,
	GL_DEPTH_COMPONENT,
	GL_R16F,
	GL_RG16F,
	GL_RGBA16F_ARB,
	GL_R32F,
	GL_RG32F,
	GL_RGBA32F_ARB
};

GLenum map_Format_Format[12] =
{
	0,
	GL_RGB,
	GL_RGBA,
	GL_RGBA,
	GL_DEPTH_STENCIL,
	GL_DEPTH_COMPONENT,
	GL_RED,
	GL_RG,
	GL_RGBA,
	GL_RED,
	GL_RG,
	GL_RGBA
};

GLenum map_Format_Type[12] =
{
	0,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_INT_8_8_8_8_REV,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_INT_24_8,
	GL_FLOAT,
	GL_HALF_FLOAT,
	GL_HALF_FLOAT,
	GL_HALF_FLOAT,
	GL_FLOAT,
	GL_FLOAT,
	GL_FLOAT
};

static void GLReadString(FILE* f, char* buff)
{
	size_t ind = 0;
	char ch = fgetc(f);

	while( ch != '\n' )
	{
		buff[ind] = ch;
		ch = fgetc(f);
		++ind;
	}

	buff[ind] = '\0';
}

// *****************************************************************************************************************************
//
// OpenGLAABox impl
//
// *****************************************************************************************************************************

#define ASGN_IF(a, op, b) \
	if( (a) op (b) ) (a) = (b);

OpenGLAABox::OpenGLAABox()
{
	Min[0] = Min[1] = Min[2] = FLT_MAX;
	Max[0] = Max[1] = Max[2] = -FLT_MAX;
}

void OpenGLAABox::Add(float x, float y, float z)
{
	ASGN_IF(Max[0], <, x);
	ASGN_IF(Max[1], <, y);
	ASGN_IF(Max[2], <, z);

	ASGN_IF(Min[0], >, x);
	ASGN_IF(Min[1], >, y);
	ASGN_IF(Min[2], >, z);
}

void OpenGLAABox::Add(float v[3])
{
	ASGN_IF(Max[0], <, v[0]);
	ASGN_IF(Max[1], <, v[1]);
	ASGN_IF(Max[2], <, v[2]);

	ASGN_IF(Min[0], >, v[0]);
	ASGN_IF(Min[1], >, v[1]);
	ASGN_IF(Min[2], >, v[2]);
}

void OpenGLAABox::GetCenter(float out[3])
{
	out[0] = (Min[0] + Max[0]) * 0.5f;
	out[1] = (Min[1] + Max[1]) * 0.5f;
	out[2] = (Min[2] + Max[2]) * 0.5f;
}

void OpenGLAABox::GetHalfSize(float out[3])
{
	out[0] = (Max[0] - Min[0]) * 0.5f;
	out[1] = (Max[1] - Min[1]) * 0.5f;
	out[2] = (Max[2] - Min[2]) * 0.5f;
}

void OpenGLAABox::TransformAxisAligned(float traf[16])
{
	float vertices[8][3] =
	{
		{ Min[0], Min[1], Min[2] },
		{ Min[0], Min[1], Max[2] },
		{ Min[0], Max[1], Min[2] },
		{ Min[0], Max[1], Max[2] },
		{ Max[0], Min[1], Min[2] },
		{ Max[0], Min[1], Max[2] },
		{ Max[0], Max[1], Min[2] },
		{ Max[0], Max[1], Max[2] }
	};
	
	for( int i = 0; i < 8; ++i )
		GLVec3TransformCoord(vertices[i], vertices[i], traf);

	Min[0] = Min[1] = Min[2] = FLT_MAX;
	Max[0] = Max[1] = Max[2] = -FLT_MAX;

	for( int i = 0; i < 8; ++i )
		Add(vertices[i]);
}

float OpenGLAABox::Nearest(float from[4]) const
{
#define FAST_DISTANCE(x, y, z, p, op) \
	d = p[0] * x + p[1] * y + p[2] * z + p[3]; \
	ASGN_IF(dist, op, d);
// END

	float d, dist = FLT_MAX;

	FAST_DISTANCE(Min[0], Min[1], Min[2], from, >);
	FAST_DISTANCE(Min[0], Min[1], Max[2], from, >);
	FAST_DISTANCE(Min[0], Max[1], Min[2], from, >);
	FAST_DISTANCE(Min[0], Max[1], Max[2], from, >);
	FAST_DISTANCE(Max[0], Min[1], Min[2], from, >);
	FAST_DISTANCE(Max[0], Min[1], Max[2], from, >);
	FAST_DISTANCE(Max[0], Max[1], Min[2], from, >);
	FAST_DISTANCE(Max[0], Max[1], Max[2], from, >);

	return dist;
}

float OpenGLAABox::Farthest(float from[4]) const
{
	float d, dist = -FLT_MAX;

	FAST_DISTANCE(Min[0], Min[1], Min[2], from, <);
	FAST_DISTANCE(Min[0], Min[1], Max[2], from, <);
	FAST_DISTANCE(Min[0], Max[1], Min[2], from, <);
	FAST_DISTANCE(Min[0], Max[1], Max[2], from, <);
	FAST_DISTANCE(Max[0], Min[1], Min[2], from, <);
	FAST_DISTANCE(Max[0], Min[1], Max[2], from, <);
	FAST_DISTANCE(Max[0], Max[1], Min[2], from, <);
	FAST_DISTANCE(Max[0], Max[1], Max[2], from, <);

	return dist;
}

// *****************************************************************************************************************************
//
// OpenGLMesh impl
//
// *****************************************************************************************************************************

OpenGLMesh::OpenGLMesh()
{
	numvertices		= 0;
	numindices		= 0;
	subsettable		= 0;
	vertexbuffer	= 0;
	indexbuffer		= 0;
	vertexlayout	= 0;

	vertexdata_locked.ptr	= 0;
	indexdata_locked.ptr	= 0;
}

OpenGLMesh::~OpenGLMesh()
{
	Destroy();
}

void OpenGLMesh::Destroy()
{
	if( vertexbuffer )
	{
		glDeleteBuffers(1, &vertexbuffer);
		vertexbuffer = 0;
	}

	if( indexbuffer )
	{
		glDeleteBuffers(1, &indexbuffer);
		indexbuffer = 0;
	}

	if( vertexlayout )
	{
		glDeleteVertexArrays(1, &vertexlayout);
		vertexlayout = 0;
	}

	if( subsettable )
	{
		free(subsettable);
		subsettable = 0;
	}
}

bool OpenGLMesh::LockVertexBuffer(GLuint flags, void** data)
{
	vertexdata_locked.ptr = malloc(numvertices * vertexdecl.stride);
	vertexdata_locked.flags = flags;

	if( !vertexdata_locked.ptr )
		return false;

	(*data) = vertexdata_locked.ptr;
	return true;
}

bool OpenGLMesh::LockIndexBuffer(GLuint flags, void** data)
{
	size_t istride = ((numvertices >= 0xffff) ? 4 : 2);

	indexdata_locked.ptr = malloc(numindices * istride);
	indexdata_locked.flags = flags;

	if( !indexdata_locked.ptr )
		return false;

	(*data) = indexdata_locked.ptr;
	return true;
}

void OpenGLMesh::DrawSubset(GLuint subset)
{
	glBindVertexArray(vertexlayout);
	glDrawElements(GL_TRIANGLES, numindices, (numvertices >= 0xffff) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}

void OpenGLMesh::UnlockVertexBuffer()
{
	if( vertexdata_locked.ptr && vertexbuffer != 0 )
	{
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, numvertices * vertexdecl.stride, vertexdata_locked.ptr, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		free(vertexdata_locked.ptr);
		vertexdata_locked.ptr = 0;
	}
}

void OpenGLMesh::UnlockIndexBuffer()
{
	if( indexdata_locked.ptr && indexbuffer != 0 )
	{
		size_t istride = ((numvertices >= 0xffff) ? 4 : 2);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, numindices * istride, indexdata_locked.ptr, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		free(indexdata_locked.ptr);
		indexdata_locked.ptr = 0;
	}
}

void OpenGLMesh::SetAttributeTable(const OpenGLAttributeRange* table, GLuint size)
{
	if( subsettable )
		free(subsettable);

	subsettable = (OpenGLAttributeRange*)malloc(size * sizeof(OpenGLAttributeRange));
	memcpy(subsettable, table, size * sizeof(OpenGLAttributeRange));
}

// *****************************************************************************************************************************
//
// OpenGLEffect impl
//
// *****************************************************************************************************************************

OpenGLEffect::OpenGLEffect()
{
	floatvalues	= 0;
	intvalues	= 0;
	floatcap	= 0;
	floatsize	= 0;
	intcap		= 0;
	intsize		= 0;
	program		= 0;
}

OpenGLEffect::~OpenGLEffect()
{
	Destroy();
}

void OpenGLEffect::Destroy()
{
	if( floatvalues )
		delete[] floatvalues;

	if( intvalues )
		delete[] intvalues;

	floatvalues = 0;
	intvalues = 0;

	if( program )
		glDeleteProgram(program);

	program = 0;
}

void OpenGLEffect::AddUniform(const char* name, GLuint location, GLuint count, GLenum type)
{
	Uniform uni;

	if( strlen(name) >= sizeof(uni.Name) )
		throw 1;

	strcpy(uni.Name, name);

	if( type == GL_FLOAT_MAT4 )
		count = 4;

	uni.Type = type;
	uni.RegisterCount = count;
	uni.Location = location;
	uni.Changed = true;

	if( type == GL_FLOAT || (type >= GL_FLOAT_VEC2 && type <= GL_FLOAT_VEC4) || type == GL_FLOAT_MAT4 )
	{
		uni.StartRegister = floatsize;

		if( floatsize + count > floatcap )
		{
			unsigned int newcap = max(floatsize + count, floatsize + 8);

			floatvalues = (float*)realloc(floatvalues, newcap * 4 * sizeof(float));
			floatcap = newcap;
		}

		float* reg = (floatvalues + uni.StartRegister * 4);
		memset(reg, 0, uni.RegisterCount * 4 * sizeof(float));

		floatsize += count;
	}
	else if( type == GL_INT || (type >= GL_INT_VEC2 && type <= GL_INT_VEC4) || type == GL_SAMPLER_2D || type == GL_IMAGE_2D )
	{
		uni.StartRegister = intsize;

		if( intsize + count > intcap )
		{
			unsigned int newcap = max(intsize + count, intsize + 8);

			intvalues = (int*)realloc(intvalues, newcap * 4 * sizeof(int));
			intcap = newcap;
		}

		int* reg = (intvalues + uni.StartRegister * 4);
		memset(reg, 0, uni.RegisterCount * 4 * sizeof(int));

		intsize += count;
	}
	else
		// not handled
		throw 1;

	uniforms.insert(uni);
}

void OpenGLEffect::BindAttributes()
{
	typedef std::map<std::string, GLuint> semanticmap;

	if( program == 0 )
		return;

	semanticmap	attribmap;
	GLint		count;
	GLenum		type;
	GLint		size, loc;
	GLchar		attribname[256];

	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &count);

	attribmap["my_Position"]	= GLDECLUSAGE_POSITION;
	attribmap["my_Normal"]		= GLDECLUSAGE_NORMAL;
	attribmap["my_Tangent"]		= GLDECLUSAGE_TANGENT;
	attribmap["my_Binormal"]	= GLDECLUSAGE_BINORMAL;
	attribmap["my_Color"]		= GLDECLUSAGE_COLOR;
	attribmap["my_Texcoord0"]	= GLDECLUSAGE_TEXCOORD;
	attribmap["my_Texcoord1"]	= GLDECLUSAGE_TEXCOORD + 10;
	attribmap["my_Texcoord2"]	= GLDECLUSAGE_TEXCOORD + 11;
	attribmap["my_Texcoord3"]	= GLDECLUSAGE_TEXCOORD + 12;
	attribmap["my_Texcoord4"]	= GLDECLUSAGE_TEXCOORD + 13;
	attribmap["my_Texcoord5"]	= GLDECLUSAGE_TEXCOORD + 14;
	attribmap["my_Texcoord6"]	= GLDECLUSAGE_TEXCOORD + 15;
	attribmap["my_Texcoord7"]	= GLDECLUSAGE_TEXCOORD + 16;

	for( int i = 0; i < count; ++i )
	{
		memset(attribname, 0, sizeof(attribname));

		glGetActiveAttrib(program, i, 256, NULL, &size, &type, attribname);
		loc = glGetAttribLocation(program, attribname);

		semanticmap::iterator it = attribmap.find(qstring(attribname));

		if( loc == -1 || it == attribmap.end() )
		{
			std::cout <<
				"Invalid attribute found. Use the my_<semantic> syntax!\n";
		}
		else
			glBindAttribLocation(program, it->second, attribname);
	}

	// bind fragment shader output
	GLint numrts;

	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &numrts);
	glBindFragDataLocation(program, 0, "my_FragColor0");

	if( numrts > 1 )
		glBindFragDataLocation(program, 1, "my_FragColor1");

	if( numrts > 2 )
		glBindFragDataLocation(program, 2, "my_FragColor2");
		
	if( numrts > 3 )
		glBindFragDataLocation(program, 3, "my_FragColor3");

	// relink
	glLinkProgram(program);
}

void OpenGLEffect::QueryUniforms()
{
	GLint		count;
	GLenum		type;
	GLsizei		length;
	GLint		size, loc;
	GLchar		uniname[256];

	if( program == 0 )
		return;

	memset(uniname, 0, sizeof(uniname));
	uniforms.clear();

	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

	// uniforms
	for( int i = 0; i < count; ++i )
	{
		memset(uniname, 0, sizeof(uniname));

		glGetActiveUniform(program, i, 256, &length, &size, &type, uniname);
		loc = glGetUniformLocation(program, uniname);

		for( int j = 0; j < length; ++j )
		{
			if( uniname[j] == '[' )
				uniname[j] = '\0';
		}

		if( loc == -1 )
			continue;

		AddUniform(uniname, loc, size, type);
	}
}

void OpenGLEffect::Begin()
{
	glUseProgram(program);
	CommitChanges();
}

void OpenGLEffect::CommitChanges()
{
	for( size_t i = 0; i < uniforms.size(); ++i )
	{
		const Uniform& uni = uniforms[i];
		float* floatdata = (floatvalues + uni.StartRegister * 4);
		int* intdata = (intvalues + uni.StartRegister * 4);

		if( !uni.Changed )
			continue;

		uni.Changed = false;

		switch( uni.Type )
		{
		case GL_FLOAT:
			glUniform1fv(uni.Location, uni.RegisterCount, floatdata);
			break;

		case GL_FLOAT_VEC2:
			glUniform2fv(uni.Location, uni.RegisterCount, floatdata);
			break;

		case GL_FLOAT_VEC3:
			glUniform3fv(uni.Location, uni.RegisterCount, floatdata);
			break;

		case GL_FLOAT_VEC4:
			glUniform4fv(uni.Location, uni.RegisterCount, floatdata);
			break;

		case GL_FLOAT_MAT4:
			glUniformMatrix4fv(uni.Location, uni.RegisterCount / 4, false, floatdata);
			break;

		case GL_INT:
		case GL_SAMPLER_2D:
			glUniform1i(uni.Location, intdata[0]);
			break;

		default:
			break;
		}
	}
}

void OpenGLEffect::End()
{
	// do nothing
}

void OpenGLEffect::SetMatrix(const char* name, float* value)
{
	SetVector(name, value);
}

void OpenGLEffect::SetVector(const char* name, float* value)
{
	Uniform test;
	strcpy(test.Name, name);

	size_t id = uniforms.find(test);

	if( id < uniforms.size() )
	{
		const Uniform& uni = uniforms[id];
		float* reg = (floatvalues + uni.StartRegister * 4);

		memcpy(reg, value, uni.RegisterCount * 4 * sizeof(float));
		uni.Changed = true;
	}
}

void OpenGLEffect::SetFloat(const char* name, float value)
{
	Uniform test;
	strcpy(test.Name, name);

	size_t id = uniforms.find(test);

	if( id < uniforms.size() )
	{
		const Uniform& uni = uniforms[id];
		float* reg = (floatvalues + uni.StartRegister * 4);

		reg[0] = value;
		uni.Changed = true;
	}
}

void OpenGLEffect::SetInt(const char* name, int value)
{
	Uniform test;
	strcpy(test.Name, name);

	size_t id = uniforms.find(test);

	if( id < uniforms.size() )
	{
		const Uniform& uni = uniforms[id];
		int* reg = (intvalues + uni.StartRegister * 4);

		reg[0] = value;
		uni.Changed = true;
	}
}

// *****************************************************************************************************************************
//
// OpenGLFrameBuffer impl
//
// *****************************************************************************************************************************

OpenGLFramebuffer::OpenGLFramebuffer(GLuint width, GLuint height)
{
	glGenFramebuffers(1, &fboid);

	sizex = width;
	sizey = height;
}

OpenGLFramebuffer::~OpenGLFramebuffer()
{
	for( int i = 0; i < 8; ++i )
	{
		if( rendertargets[i].id != 0 )
		{
			if( rendertargets[i].type == 0 )
				glDeleteRenderbuffers(1, &rendertargets[i].id);
			else
				glDeleteTextures(1, &rendertargets[i].id);
		}
	}

	if( depthstencil.id != 0 )
	{
		if( depthstencil.type == 0 )
			glDeleteRenderbuffers(1, &depthstencil.id);
		else
			glDeleteTextures(1, &depthstencil.id);
	}

	glDeleteFramebuffers(1, &fboid);
}

bool OpenGLFramebuffer::AttachRenderbuffer(GLenum target, OpenGLFormat format)
{
	Attachment* attach = 0;

	if( target == GL_DEPTH_ATTACHMENT || target == GL_DEPTH_STENCIL_ATTACHMENT )
		attach = &depthstencil;
	else if( target >= GL_COLOR_ATTACHMENT0 && target < GL_COLOR_ATTACHMENT8 )
		attach = &rendertargets[target - GL_COLOR_ATTACHMENT0];
	else
	{
		std::cout << "Target is invalid!\n";
		return false;
	}

	if( attach->id != 0 )
	{
		std::cout << "Already attached to this target!\n";
		return false;
	}

	glGenRenderbuffers(1, &attach->id);
	
	glBindFramebuffer(GL_FRAMEBUFFER, fboid);
	glBindRenderbuffer(GL_RENDERBUFFER, attach->id);

	glRenderbufferStorage(GL_RENDERBUFFER, map_Format_Internal[format], sizex, sizey);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, target, GL_RENDERBUFFER, attach->id);

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	attach->type = 0;

	return true;
}

bool OpenGLFramebuffer::AttachTexture(GLenum target, OpenGLFormat format)
{
	Attachment* attach = 0;

	if( target == GL_DEPTH_ATTACHMENT || target == GL_DEPTH_STENCIL_ATTACHMENT )
		attach = &depthstencil;
	else if( target >= GL_COLOR_ATTACHMENT0 && target < GL_COLOR_ATTACHMENT8 )
		attach = &rendertargets[target - GL_COLOR_ATTACHMENT0];
	else
	{
		std::cout << "Target is invalid!\n";
		return false;
	}

	if( attach->id != 0 )
	{
		std::cout << "Already attached to this target!\n";
		return false;
	}

	glGenTextures(1, &attach->id);

	glBindFramebuffer(GL_FRAMEBUFFER, fboid);
	glBindTexture(GL_TEXTURE_2D, attach->id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, map_Format_Internal[format], sizex, sizey, 0, map_Format_Format[format], map_Format_Type[format], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D, attach->id, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	attach->type = 1;

	return true;
}

bool OpenGLFramebuffer::Validate()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fboid);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	switch( status )
	{
	case GL_FRAMEBUFFER_COMPLETE:
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		std::cout << "OpenGLFramebuffer::Validate(): GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT!\n";
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		std::cout << "OpenGLFramebuffer::Validate(): GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS!\n";
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		std::cout << "OpenGLFramebuffer::Validate(): GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT!\n";
		break;

	default:
		std::cout << "OpenGLFramebuffer::Validate(): Unknown error!\n";
		break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return (status == GL_FRAMEBUFFER_COMPLETE);
}

void OpenGLFramebuffer::Set()
{
	GLenum buffs[8];
	GLsizei count = 0;

	for( int i = 0; i < 8; ++i )
	{
		if( rendertargets[i].id != 0 )
		{
			buffs[i] = GL_COLOR_ATTACHMENT0 + i;
			count = i;
		}
		else
			buffs[i] = GL_NONE;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, fboid);

	if( count > 0 )
		glDrawBuffers(count, buffs);
}

void OpenGLFramebuffer::Unset()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
}

// *****************************************************************************************************************************
//
// Functions impl
//
// *****************************************************************************************************************************

bool GLCreateMesh(GLuint numfaces, GLuint numvertices, GLuint options, OpenGLVertexElement* decl, OpenGLMesh** mesh)
{
	OpenGLMesh* glmesh = new OpenGLMesh();

	glGenBuffers(1, &glmesh->vertexbuffer);
	glGenBuffers(1, &glmesh->indexbuffer);
	glGenVertexArrays(1, &glmesh->vertexlayout);

	glmesh->numvertices = numvertices;
	glmesh->numindices = numfaces * 3;
	glmesh->vertexdecl.stride = 0;

	// create vertex layout
	glBindVertexArray(glmesh->vertexlayout);
	{
		glBindBuffer(GL_ARRAY_BUFFER, glmesh->vertexbuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glmesh->indexbuffer);

		// calculate stride
		for( int i = 0; i < 16; ++i )
		{
			OpenGLVertexElement& elem = decl[i];

			if( elem.Stream == 0xff )
				break;

			switch( elem.Type )
			{
			case GLDECLTYPE_GLCOLOR:
			case GLDECLTYPE_FLOAT1:		glmesh->vertexdecl.stride += 4;		break;
			case GLDECLTYPE_FLOAT2:		glmesh->vertexdecl.stride += 8;		break;
			case GLDECLTYPE_FLOAT3:		glmesh->vertexdecl.stride += 12;	break;
			case GLDECLTYPE_FLOAT4:		glmesh->vertexdecl.stride += 16;	break;

			default:
				break;
			}
		}

		// bind locations
		for( int i = 0; i < 16; ++i )
		{
			OpenGLVertexElement& elem = decl[i];

			if( elem.Stream == 0xff )
				break;

			glEnableVertexAttribArray(elem.Usage);

			switch( elem.Usage )
			{
			case GLDECLUSAGE_POSITION:
				glVertexAttribPointer(elem.Usage, (elem.Type == GLDECLTYPE_FLOAT4 ? 4 : 3), GL_FLOAT, GL_FALSE, glmesh->vertexdecl.stride, (const GLvoid*)elem.Offset);
				break;

			case GLDECLUSAGE_COLOR:
				glVertexAttribPointer(elem.Usage, 4, GL_UNSIGNED_BYTE, GL_TRUE, glmesh->vertexdecl.stride, (const GLvoid*)elem.Offset);
				break;

			case GLDECLUSAGE_NORMAL:
				glVertexAttribPointer(elem.Usage, 3, GL_FLOAT, GL_FALSE, glmesh->vertexdecl.stride, (const GLvoid*)elem.Offset);
				break;

			case GLDECLUSAGE_TEXCOORD:
				// haaack...
				glVertexAttribPointer(elem.Usage + elem.UsageIndex, (elem.Type + 1), GL_FLOAT, GL_FALSE, glmesh->vertexdecl.stride, (const GLvoid*)elem.Offset);
				break;

			// TODO:

			default:
				std::cout << "Unhandled layout element...\n";
				break;
			}
		}
	}
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	(*mesh) = glmesh;
	return true;
}

bool GLLoadMeshFromQM(const char* file, OpenGLMaterial** materials, GLuint* nummaterials, OpenGLMesh** mesh)
{
	static const unsigned char usages[] =
	{
		GLDECLUSAGE_POSITION,
		GLDECLUSAGE_POSITIONT,
		GLDECLUSAGE_COLOR,
		GLDECLUSAGE_BLENDWEIGHT,
		GLDECLUSAGE_BLENDINDICES,
		GLDECLUSAGE_NORMAL,
		GLDECLUSAGE_TEXCOORD,
		GLDECLUSAGE_TANGENT,
		GLDECLUSAGE_BINORMAL,
		GLDECLUSAGE_PSIZE,
		GLDECLUSAGE_TESSFACTOR
	};

	static const unsigned short elemsizes[6] =
	{
		1, 2, 3, 4, 4, 4
	};

	static const unsigned short elemstrides[6] =
	{
		4, 4, 4, 4, 1, 1
	};

	OpenGLVertexElement*	decl;
	OpenGLAttributeRange*	table;
	OpenGLColor			color;
	FILE*				infile = 0;
	float				bbmin[3];
	float				bbmax[3];
	unsigned int		unused;
	unsigned int		version;
	unsigned int		numindices;
	unsigned int		numvertices;
	unsigned int		vstride;
	unsigned int		istride;
	unsigned int		numsubsets;
	unsigned int		numelems;
	unsigned short		tmp16;
	unsigned char		tmp8;
	void*				data;
	char				buff[256];
	bool				success;

#ifdef _MSC_VER
	fopen_s(&infile, file, "rb");
#else
	infile = fopen(file, "rb");
#endif

	if( !infile )
		return false;

	fread(&unused, 4, 1, infile);
	fread(&numindices, 4, 1, infile);
	fread(&istride, 4, 1, infile);
	fread(&numsubsets, 4, 1, infile);

	version = unused >> 16;

	fread(&numvertices, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);
	fread(&unused, 4, 1, infile);

	table = new OpenGLAttributeRange[numsubsets];

	// vertex declaration
	fread(&numelems, 4, 1, infile);
	decl = new OpenGLVertexElement[numelems + 1];

	vstride = 0;

	for( unsigned int i = 0; i < numelems; ++i )
	{
		fread(&tmp16, 2, 1, infile);
		decl[i].Stream = tmp16;

		fread(&tmp8, 1, 1, infile);
		decl[i].Usage = usages[tmp8];

		fread(&tmp8, 1, 1, infile);
		decl[i].Type = tmp8;

		fread(&tmp8, 1, 1, infile);
		decl[i].UsageIndex = tmp8;
		decl[i].Offset = vstride;

		vstride += elemsizes[decl[i].Type] * elemstrides[decl[i].Type];
	}

	decl[numelems].Stream = 0xff;
	decl[numelems].Offset = 0;
	decl[numelems].Type = 0;
	decl[numelems].Usage = 0;
	decl[numelems].UsageIndex = 0;

	// create mesh
	success = GLCreateMesh(numindices / 3, numvertices, 0, decl, mesh);

	if( !success )
		goto _fail;

	(*mesh)->LockVertexBuffer(0, &data);
	fread(data, vstride, numvertices, infile);
	(*mesh)->UnlockVertexBuffer();

	(*mesh)->LockIndexBuffer(0, &data);
	fread(data, istride, numindices, infile);
	(*mesh)->UnlockIndexBuffer();

	if( version > 1 )
	{
		fread(&unused, 4, 1, infile);

		if( unused > 0 )
			fseek(infile, 8 * unused, SEEK_CUR);
	}

	// attribute table
	(*materials) = new OpenGLMaterial[numsubsets];

	for( unsigned int i = 0; i < numsubsets; ++i )
	{
		OpenGLAttributeRange& subset = table[i];
		OpenGLMaterial& mat = (*materials)[i];

		mat.TextureFile = 0;
		subset.AttribId = i;

		fread(&subset.FaceStart, 4, 1, infile);
		fread(&subset.VertexStart, 4, 1, infile);
		fread(&subset.VertexCount, 4, 1, infile);
		fread(&subset.FaceCount, 4, 1, infile);

		subset.FaceCount /= 3;
		subset.FaceStart /= 3;

		fread(bbmin, sizeof(float), 3, infile);
		fread(bbmax, sizeof(float), 3, infile);

		(*mesh)->boundingbox.Add(bbmin);
		(*mesh)->boundingbox.Add(bbmax);

		GLReadString(infile, buff);
		GLReadString(infile, buff);

		bool hasmaterial = (buff[1] != ',');

		if( hasmaterial )
		{
			fread(&color, sizeof(OpenGLColor), 1, infile);
			mat.Ambient = color;

			fread(&color, sizeof(OpenGLColor), 1, infile);
			mat.Diffuse = color;

			fread(&color, sizeof(OpenGLColor), 1, infile);
			mat.Specular = color;

			fread(&color, sizeof(OpenGLColor), 1, infile);
			mat.Emissive = color;

			fread(&mat.Power, sizeof(float), 1, infile);
			fread(&mat.Diffuse.a, sizeof(float), 1, infile);

			fread(&unused, 4, 1, infile);
			GLReadString(infile, buff);

			if( buff[1] != ',' )
			{
				unused = strlen(buff);

				mat.TextureFile = new char[unused + 1];
				memcpy(mat.TextureFile, buff, unused);
				mat.TextureFile[unused] = 0;
			}

			GLReadString(infile, buff);
			GLReadString(infile, buff);
			GLReadString(infile, buff);
			GLReadString(infile, buff);
			GLReadString(infile, buff);
			GLReadString(infile, buff);
			GLReadString(infile, buff);
		}
		else
		{
			color = OpenGLColor(1, 1, 1, 1);

			memcpy(&mat.Ambient, &color, 4 * sizeof(float));
			memcpy(&mat.Diffuse, &color, 4 * sizeof(float));
			memcpy(&mat.Specular, &color, 4 * sizeof(float));

			color = OpenGLColor(0, 0, 0, 1);
			memcpy(&mat.Emissive, &color, 4 * sizeof(float));

			mat.Power = 80.0f;
		}

		GLReadString(infile, buff);

		if( buff[1] != ',' && mat.TextureFile == 0 )
		{
			unused = strlen(buff);

			mat.TextureFile = new char[unused + 1];
			memcpy(mat.TextureFile, buff, unused);
			mat.TextureFile[unused] = 0;
		}

		GLReadString(infile, buff);
		GLReadString(infile, buff);
		GLReadString(infile, buff);
		GLReadString(infile, buff);
		GLReadString(infile, buff);
		GLReadString(infile, buff);
		GLReadString(infile, buff);
	}

	// attribute buffer
	(*mesh)->SetAttributeTable(table, numsubsets);
	*nummaterials = numsubsets;

_fail:
	delete[] decl;
	delete[] table;

	fclose(infile);
	return success;
}

bool GLCreateEffectFromFile(const char* vsfile, const char* psfile, OpenGLEffect** effect)
{
	char			log[1024];
	OpenGLEffect*	neweffect;
	GLuint			vertexshader = 0;
	GLuint			fragmentshader = 0;
	FILE*			infile = 0;
	GLint			success;
	GLint			length;
	char*			source;

	// vertex shader
	if( !(infile = fopen(vsfile, "rb")) )
		return false;

	fseek(infile, 0, SEEK_END);
	length = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	source = new char[length];
	vertexshader = glCreateShader(GL_VERTEX_SHADER);

	fread(source, 1, length, infile);
	fclose(infile);

	glShaderSource(vertexshader, 1, (const GLcharARB**)&source, &length);
	glCompileShader(vertexshader);
	glGetShaderiv(vertexshader, GL_COMPILE_STATUS, &success);

	if( success != GL_TRUE )
	{
		glGetShaderInfoLog(vertexshader, 1024, &length, log);
		log[length] = 0;

		std::cout << log << "\n";

		delete[] source;
		glDeleteShader(vertexshader);

		return false;
	}

	delete[] source;

	// fragment shader
	if( !(infile = fopen(psfile, "rb")) )
		return false;

	fseek(infile, 0, SEEK_END);
	length = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	source = new char[length];
	fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);

	fread(source, 1, length, infile);
	fclose(infile);

	glShaderSource(fragmentshader, 1, (const GLcharARB**)&source, &length);
	glCompileShader(fragmentshader);
	glGetShaderiv(fragmentshader, GL_COMPILE_STATUS, &success);

	if( success != GL_TRUE )
	{
		glGetShaderInfoLog(fragmentshader, 1024, &length, log);
		log[length] = 0;

		std::cout << log << "\n";

		delete[] source;

		glDeleteShader(vertexshader);
		glDeleteShader(fragmentshader);

		return false;
	}

	delete[] source;

	// program
	neweffect = new OpenGLEffect();
	neweffect->program = glCreateProgram();

	glAttachShader(neweffect->program, vertexshader);
	glAttachShader(neweffect->program, fragmentshader);
	glLinkProgram(neweffect->program);
	glGetProgramiv(neweffect->program, GL_LINK_STATUS, &success);

	if( success != GL_TRUE )
	{
		glGetProgramInfoLog(neweffect->program, 1024, &length, log);
		log[length] = 0;

		std::cout << log << "\n";

		glDeleteProgram(neweffect->program);
		delete neweffect;

		return false;
	}

	neweffect->BindAttributes();
	neweffect->QueryUniforms();

	glDeleteShader(vertexshader);
	glDeleteShader(fragmentshader);

	(*effect) = neweffect;
	return true;
}

bool GLCreateComputeProgramFromFile(const char* csfile, const char* defines, OpenGLEffect** effect)
{
	char			log[1024];
	OpenGLEffect*	neweffect;
	GLuint			shader = 0;
	FILE*			infile = 0;
	GLint			success;
	GLint			length;
	int				deflength = 0;
	char*			source;

	if( !(infile = fopen(csfile, "rb")) )
		return false;

	fseek(infile, 0, SEEK_END);
	length = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	if( defines )
		deflength = strlen(defines);

	source = new char[length + deflength];
	shader = glCreateShader(GL_COMPUTE_SHADER);

	if( defines )
		memcpy(source, defines, deflength);

	fread(source + deflength, 1, length, infile);
	fclose(infile);

	glShaderSource(shader, 1, (const GLcharARB**)&source, &length);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if( success != GL_TRUE )
	{
		glGetShaderInfoLog(shader, 1024, &length, log);
		log[length] = 0;

		std::cout << log << "\n";

		delete[] source;
		glDeleteShader(shader);

		return false;
	}

	delete[] source;

	// program
	neweffect = new OpenGLEffect();
	neweffect->program = glCreateProgram();

	glAttachShader(neweffect->program, shader);
	glLinkProgram(neweffect->program);
	glGetProgramiv(neweffect->program, GL_LINK_STATUS, &success);

	if( success != GL_TRUE )
	{
		glGetProgramInfoLog(neweffect->program, 1024, &length, log);
		log[length] = 0;

		std::cout << log << "\n";

		glDeleteProgram(neweffect->program);
		delete neweffect;

		return false;
	}

	neweffect->QueryUniforms();
	glDeleteShader(shader);

	(*effect) = neweffect;
	return true;
}

// *****************************************************************************************************************************
//
// Math functions impl
//
// *****************************************************************************************************************************

int isqrt(int n)
{
	int b = 0;

	while( n >= 0 )
	{
		n = n - b;
		b = b + 1;
		n = n - b;
	}

	return b - 1;
}

float GLVec3Dot(float a[3], float b[3])
{
	return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

float GLVec3Length(float a[3])
{
	return sqrtf(GLVec3Dot(a, a));
}

void GLVec3Normalize(float a[3])
{
	float il = 1.0f / GLVec3Length(a);

	a[0] *= il;
	a[1] *= il;
	a[2] *= il;
}

void GLVec3Cross(float out[3], float a[3], float b[3])
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

void GLVec3TransformCoord(float out[3], float v[3], float m[16])
{
	float tmp[4];

	tmp[0] = v[0] * m[0] + v[1] * m[4] + v[2] * m[8] + m[12];
	tmp[1] = v[0] * m[1] + v[1] * m[5] + v[2] * m[9] + m[13];
	tmp[2] = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + m[14];
	tmp[3] = v[0] * m[3] + v[1] * m[7] + v[2] * m[11] + m[15];

	out[0] = tmp[0] / tmp[3];
	out[1] = tmp[1] / tmp[3];
	out[2] = tmp[2] / tmp[3];
}

void GLVec4Transform(float out[4], float v[4], float m[16])
{
	float tmp[4];

	tmp[0] = v[0] * m[0] + v[1] * m[4] + v[2] * m[8] + v[3] * m[12];
	tmp[1] = v[0] * m[1] + v[1] * m[5] + v[2] * m[9] + v[3] * m[13];
	tmp[2] = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + v[3] * m[14];
	tmp[3] = v[0] * m[3] + v[1] * m[7] + v[2] * m[11] + v[3] * m[15];

	out[0] = tmp[0];
	out[1] = tmp[1];
	out[2] = tmp[2];
	out[3] = tmp[3];
}

void GLVec4TransformTranspose(float out[4], float m[16], float v[4])
{
	float tmp[4];

	tmp[0] = v[0] * m[0] + v[1] * m[1] + v[2] * m[2] + v[3] * m[3];
	tmp[1] = v[0] * m[4] + v[1] * m[5] + v[2] * m[6] + v[3] * m[7];
	tmp[2] = v[0] * m[8] + v[1] * m[9] + v[2] * m[10] + v[3] * m[11];
	tmp[3] = v[0] * m[12] + v[1] * m[13] + v[2] * m[14] + v[3] * m[15];

	out[0] = tmp[0];
	out[1] = tmp[1];
	out[2] = tmp[2];
	out[3] = tmp[3];
}

void GLPlaneNormalize(float p[4])
{
	float length = GLVec3Length(p);

	p[0] /= length;
	p[1] /= length;
	p[2] /= length;
	p[3] /= length;
}

void GLMatrixLookAtRH(float out[16], float eye[3], float look[3], float up[3])
{
	float x[3], y[3], z[3];

	z[0] = look[0] - eye[0];
	z[1] = look[1] - eye[1];
	z[2] = look[2] - eye[2];

	GLVec3Normalize(z);
	GLVec3Cross(x, z, up);

	GLVec3Normalize(x);
	GLVec3Cross(y, x, z);

	out[0] = x[0];		out[1] = y[0];		out[2] = -z[0];		out[3] = 0.0f;
	out[4] = x[1];		out[5] = y[1];		out[6] = -z[1];		out[7] = 0.0f;
	out[8] = x[2];		out[9] = y[2];		out[10] = -z[2];	out[11] = 0.0f;

	out[12] = GLVec3Dot(x, eye);
	out[13] = GLVec3Dot(y, eye);
	out[14] = GLVec3Dot(z, eye);
	out[15] = 1.0f;
}

void GLMatrixPerspectiveRH(float out[16], float fovy, float aspect, float nearplane, float farplane)
{
	out[5] = 1.0f / tanf(fovy / 2);
	out[0] = out[5] / aspect;

	out[1] = out[2] = out[3] = 0;
	out[4] = out[6] = out[7] = 0;
	out[8] = out[9] = 0;
	out[12] = out[13] = out[15] = 0;

	out[11] = -1.0f;

	out[10] = (farplane + nearplane) / (nearplane - farplane);
	out[14] = 2 * farplane * nearplane / (nearplane - farplane);
}

void GLMatrixMultiply(float out[16], float a[16], float b[16])
{
	float tmp[16];

	tmp[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
	tmp[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
	tmp[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
	tmp[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

	tmp[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
	tmp[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
	tmp[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
	tmp[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

	tmp[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
	tmp[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
	tmp[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
	tmp[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

	tmp[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
	tmp[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
	tmp[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
	tmp[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];

	memcpy(out, tmp, 16 * sizeof(float));
}

void GLMatrixRotationAxis(float out[16], float angle, float x, float y, float z)
{
	float u[3] = { x, y, z };

	float cosa = cosf(angle);
	float sina = sinf(angle);

	GLVec3Normalize(u);

	out[0] = cosa + u[0] * u[0] * (1.0f - cosa);
	out[1] = u[0] * u[1] * (1.0f - cosa) - u[2] * sina;
	out[2] = u[0] * u[2] * (1.0f - cosa) + u[1] * sina;
	out[3] = 0;

	out[4] = u[1] * u[0] * (1.0f - cosa) + u[2] * sina;
	out[5] = cosa + u[1] * u[1] * (1.0f - cosa);
	out[6] = u[1] * u[2] * (1.0f - cosa) - u[0] * sina;
	out[7] = 0;

	out[8] = u[2] * u[0] * (1.0f - cosa) - u[1] * sina;
	out[9] = u[2] * u[1] * (1.0f - cosa) + u[0] * sina;
	out[10] = cosa + u[2] * u[2] * (1.0f - cosa);
	out[11] = 0;

	out[12] = out[13] = out[14] = 0;
	out[15] = 1;
}

void GLMatrixIdentity(float out[16])
{
	memset(out, 0, 16 * sizeof(float));
	out[0] = out[5] = out[10] = out[15] = 1;
}

void GLFitToBox(float& outnear, float& outfar, float eye[3], float look[3], const OpenGLAABox& box)
{
	float refplane[4];
	float length;

	refplane[0] = look[0] - eye[0];
	refplane[1] = look[1] - eye[1];
	refplane[2] = look[2] - eye[2];
	refplane[3] = -(refplane[0] * eye[0] + refplane[1] * eye[1] + refplane[2] * eye[2]);

	length = GLVec3Length(refplane);

	refplane[0] /= length;
	refplane[1] /= length;
	refplane[2] /= length;
	refplane[3] /= length;

	outnear = box.Nearest(refplane) - 0.02f;	// 2mm
	outfar = box.Farthest(refplane) + 0.02f;	// 2mm

	outnear = std::max<float>(outnear, 0.1f);
	outfar = std::max<float>(outfar, 0.2f);
}
