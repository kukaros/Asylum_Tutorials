
#include "glext.h"

#include <iostream>
#include <cstdio>
#include <cmath>

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
		fread(&unused, 4, 1, infile);

		subset.FaceCount /= 3;
		subset.FaceStart /= 3;

		fseek(infile, 6 * sizeof(float), SEEK_CUR);

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
