//
//  GLViewController.m
//  CoreProfileTestMac
//
//  Created by gliptak on 4/16/13.
//  Copyright (c) 2013 Asylum. All rights reserved.
//

#import "GLViewController.h"
#import <OpenGL/gl3.h>

// helper macros
#define MYERROR(x)			{ printf("* Error: %s!\n", x); }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

// tutorial variables
GLuint texid				= 0;
GLuint vertexbuffer			= 0;
GLuint indexbuffer			= 0;
GLuint vertexshader			= 0;
GLuint pixelshader			= 0;
GLuint program				= 0;
GLuint vertexdecl			= 0;

GLuint attrib_Position		= 0;
GLuint attrib_Normal		= 0;

GLuint uniform_lightPos		= 0;
GLuint uniform_eyePos		= 0;
GLuint uniform_matWorld		= 0;
GLuint uniform_matViewProj	= 0;

const char* vscode =
{
	"#version 150\n"

	"in vec3 my_Position;\n"
	"in vec3 my_Normal;\n"

	"uniform mat4 matWorld;\n"
	"uniform mat4 matWorldInv;\n"
	"uniform mat4 matViewProj;\n"

	"uniform vec4 lightPos;\n"
	"uniform vec4 eyePos;\n"

	"out vec3 wnorm;\n"
	"out vec3 vdir;\n"
	"out vec3 ldir;\n"

	"void main()\n"
	"{\n"
	"	vec4 wpos = matWorld * vec4(my_Position, 1);\n"

	"	ldir = lightPos.xyz - wpos.xyz;\n"
	"	vdir = eyePos.xyz - wpos.xyz;\n"

	"	wnorm = (matWorld * vec4(my_Normal, 0)).xyz;\n"
	"	gl_Position = matViewProj * wpos;\n"
	"}\n"
};

const char* pscode =
{
	"#version 150\n"

	"in vec3 wnorm;\n"
	"in vec3 vdir;\n"
	"in vec3 ldir;\n"
	
	"out vec4 outColor;\n"

	"void main()\n"
	"{\n"
	"	vec3 n = normalize(wnorm);\n"
	"	vec3 l = normalize(ldir);\n"
	"	vec3 v = normalize(vdir);\n"
	"	vec3 h = normalize(v + l);\n"

	"	float d = clamp(dot(l, n), 0.0, 1.0);\n"
	"	float s = clamp(dot(h, n), 0.0, 1.0);\n"

	"	s = pow(s, 80.0);\n"

	"	outColor.rgb = vec3(d, d, d) + vec3(s, s, s);\n"
	"	outColor.a = 1.0;\n"
	"}\n"
};

typedef struct _CommonVertex
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
} CommonVertex;

float vertices[24 * sizeof(CommonVertex)] =
{
	-0.5f, -0.5f, 0.5f, 0, -1, 0, 1, 0,
	-0.5f, -0.5f, -0.5f, 0, -1, 0, 1, 1,
	0.5f, -0.5f, -0.5f, 0, -1, 0, 0, 1,
	0.5f, -0.5f, 0.5f, 0, -1, 0, 0, 0,

	-0.5f, 0.5f, 0.5f, 0, 1, 0, 0, 0,
	0.5f, 0.5f, 0.5f, 0, 1, 0, 1, 0,
	0.5f, 0.5f, -0.5f, 0, 1, 0, 1, 1,
	-0.5f, 0.5f, -0.5f, 0, 1, 0, 0, 1,

	-0.5f, -0.5f, 0.5f, 0, 0, 1, 0, 0,
	0.5f, -0.5f, 0.5f, 0, 0, 1, 1, 0,
	0.5f, 0.5f, 0.5f, 0, 0, 1, 1, 1,
	-0.5f, 0.5f, 0.5f, 0, 0, 1, 0, 1,

	0.5f, -0.5f, 0.5f, 1, 0, 0, 0, 0,
	0.5f, -0.5f, -0.5f, 1, 0, 0, 1, 0,
	0.5f, 0.5f, -0.5f, 1, 0, 0, 1, 1,
	0.5f, 0.5f, 0.5f, 1, 0, 0, 0, 1,

	0.5f, -0.5f, -0.5f, 0, 0, -1, 0, 0,
	-0.5f, -0.5f, -0.5f, 0, 0, -1, 1, 0,
	-0.5f, 0.5f, -0.5f, 0, 0, -1, 1, 1,
	0.5f, 0.5f, -0.5f, 0, 0, -1, 0, 1,

	-0.5f, -0.5f, -0.5f, -1, 0, 0, 0, 0,
	-0.5f, -0.5f, 0.5f, -1, 0, 0, 1, 0,
	-0.5f, 0.5f, 0.5f, -1, 0, 0, 1, 1,
	-0.5f, 0.5f, -0.5f, -1, 0, 0, 0, 1
};

unsigned short indices[36] =
{
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4,
	8, 9, 10, 10, 11, 8,
	12, 13, 14, 14, 15, 12,
	16, 17, 18, 18, 19, 16,
	20, 21, 22, 22, 23, 20
};

float Dot(float a[3], float b[3]) {
	return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

float Length(float a[3]) {
	return sqrtf(Dot(a, a));
}

void Normalize(float a[3])
{
	float il = 1.0f / Length(a);

	a[0] *= il;
	a[1] *= il;
	a[2] *= il;
}

void Cross(float out[3], float a[3], float b[3])
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

void LookAtRH(float out[16], float eye[3], float look[3], float up[3])
{
	float x[3], y[3], z[3];

	z[0] = look[0] - eye[0];
	z[1] = look[1] - eye[1];
	z[2] = look[2] - eye[2];

	Normalize(z);
	Cross(x, z, up);

	Normalize(x);
	Cross(y, x, z);

	out[0] = x[0];		out[1] = y[0];		out[2] = -z[0];		out[3] = 0.0f;
	out[4] = x[1];		out[5] = y[1];		out[6] = -z[1];		out[7] = 0.0f;
	out[8] = x[2];		out[9] = y[2];		out[10] = -z[2];	out[11] = 0.0f;

	out[12] = Dot(x, eye);
	out[13] = Dot(y, eye);
	out[14] = Dot(z, eye);
	out[15] = 1.0f;
}

void PerspectiveRH(float out[16], float fovy, float aspect, float nearplane, float farplane)
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

void Multiply(float out[16], float a[16], float b[16])
{
	out[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
	out[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
	out[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
	out[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

	out[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
	out[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
	out[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
	out[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

	out[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
	out[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
	out[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
	out[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

	out[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
	out[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
	out[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
	out[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
}

void RotationAxis(float out[16], float angle, float x, float y, float z)
{
	float u[3] = { x, y, z };

	float cosa = cosf(angle);
	float sina = sinf(angle);

	Normalize(u);

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

void Identity(float out[16])
{
	memset(out, 0, 16 * sizeof(float));
	out[0] = out[5] = out[10] = out[15] = 1;
}

@implementation GLViewController

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (BOOL)becomeFirstResponder
{
	return YES;
}

- (BOOL)resignFirstResponder
{
	return YES;
}

- (void)animationTimer:(NSTimer*)timer
{
	[self drawRect:[self bounds]];
}

- (void)awakeFromNib
{
	NSOpenGLPixelFormatAttribute attributes[] =
	{
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
		0
	};

	NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	NSOpenGLContext* context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];

	if (context == nil)
	{
		attributes[11] = (NSOpenGLPixelFormatAttribute)nil;

		format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
		context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
	}
	
	[self setOpenGLContext:context];
	[context makeCurrentContext];

	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* version = (const char*)glGetString(GL_VERSION);
	
	printf("%s\n%s\n", renderer, version);
	
	// setup opengl
	//glClearColor(0.4f, 0.58f, 0.93f, 1.0f);
	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	// mesh
	glGenBuffers(1, &vertexbuffer);
	glGenBuffers(1, &indexbuffer);

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(CommonVertex), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(unsigned short), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// shader
	GLint length;
	GLint success = GL_FALSE;
	char buff[1024];

	program = glCreateProgram();
	vertexshader = glCreateShader(GL_VERTEX_SHADER);
	pixelshader = glCreateShader(GL_FRAGMENT_SHADER);

	length = (GLint)strlen(vscode);
	glShaderSource(vertexshader, 1, &vscode, &length);
	glCompileShader(vertexshader);
	glGetShaderiv(vertexshader, GL_COMPILE_STATUS, &success);

	if( success != GL_TRUE )
	{
		MYERROR("awakeFromNib(): Could not compile vertex shader");
		
		glGetShaderInfoLog(vertexshader, 1024, &length, buff);
		printf("%s\n", buff);
		
		return;
	}

	length = (GLint)strlen(pscode);
	glShaderSource(pixelshader, 1, &pscode, &length);
	glCompileShader(pixelshader);
	glGetShaderiv(pixelshader, GL_COMPILE_STATUS, &success);

	if( success != GL_TRUE )
	{
		MYERROR("awakeFromNib(): Could not compile pixel shader");
		
		glGetShaderInfoLog(pixelshader, 1024, &length, buff);
		printf("%s\n", buff);
		
		return;
	}

	glAttachShader(program, vertexshader);
	glAttachShader(program, pixelshader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if( success != GL_TRUE )
	{
		glGetProgramInfoLog(program, 1024, &length, buff);
		printf("%s\n", buff);
		
		MYERROR("awakeFromNib(): Could not link shader");
		return;
	}

	glUseProgram(program);
	{
		attrib_Position = glGetAttribLocation(program, "my_Position");
		attrib_Normal = glGetAttribLocation(program, "my_Normal");

		uniform_lightPos = glGetUniformLocation(program, "lightPos");
		uniform_eyePos = glGetUniformLocation(program, "eyePos");
		uniform_matWorld = glGetUniformLocation(program, "matWorld");
		uniform_matViewProj = glGetUniformLocation(program, "matViewProj");
	}
	glUseProgram(0);

	// vertex decl
	glGenVertexArrays(1, &vertexdecl);
	glBindVertexArray(vertexdecl);
	{
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);

		glEnableVertexAttribArray(attrib_Position);
		glEnableVertexAttribArray(attrib_Normal);

		glVertexAttribPointer(attrib_Position, 3, GL_FLOAT, GL_FALSE, sizeof(CommonVertex), 0);
		glVertexAttribPointer(attrib_Normal, 3, GL_FLOAT, GL_FALSE, sizeof(CommonVertex), (const GLvoid*)(3 * sizeof(float)));
	}
	glBindVertexArray(0);

	timer = [NSTimer timerWithTimeInterval:(1.0f / 60.0f) target:self selector:@selector(animationTimer:) userInfo:nil repeats:YES];

    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSEventTrackingRunLoopMode];
}

- (void)drawRect:(NSRect)dirtyRect
{
	static float time = 0;

	float lightpos[4]	= { 6, 3, 10, 1 };
	float eye[3]		= { 0, 0, 2 };
	float look[3]		= { 0, 0, 0 };
	float up[3]			= { 0, 1, 0 };

	float view[16];
	float proj[16];
	float world[16];
	float viewproj[16];
	float tmp1[16];
	float tmp2[16];

	float screenwidth = [self bounds].size.width;
	float screenheight = [self bounds].size.height;

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	LookAtRH(view, eye, look, up);
	PerspectiveRH(proj, (60.0f * 3.14159f) / 180.f,  screenwidth / screenheight, 0.1f, 100.0f);

	Multiply(viewproj, view, proj);

	RotationAxis(tmp1, fmodf(time * 20.0f, 360.0f) * (3.14152f / 180.0f), 1, 0, 0);
	RotationAxis(tmp2, fmodf(time * 20.0f, 360.0f) * (3.14152f / 180.0f), 0, 1, 0);

	Multiply(world, tmp1, tmp2);

	time += (1.0f / 60.0f);

	glUseProgram(program);
	glUniform4fv(uniform_eyePos, 1, eye);
	glUniform4fv(uniform_lightPos, 1, lightpos);
	glUniformMatrix4fv(uniform_matWorld, 1, false, world);
	glUniformMatrix4fv(uniform_matViewProj, 1, false, viewproj);
	{
		glBindVertexArray(vertexdecl);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
		glBindVertexArray(0);
	}
	glUseProgram(0);

	[self.openGLContext flushBuffer];
}

@end
