//
//  GLViewController.m
//  CoreProfileTestMac
//
//  Created by gliptak on 4/16/13.
//  Copyright (c) 2013 Asylum. All rights reserved.
//

#import "GLViewController.h"
#import <OpenGL/gl3.h>
#import <mach/mach_time.h>
#import <time.h>

#import "../../common/glext.h"

// helper macros
#define MYERROR(x)			{ printf("* Error: %s!\n", x); }
#define V_RETURN(r, e, x)	{ if( !(x) ) { MYERROR(e); return r; }}

// tutorial variables
OpenGLMesh* mesh = 0;

GLuint vertexshader			= 0;
GLuint pixelshader			= 0;
GLuint program				= 0;

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

	OpenGLMaterial* materials = 0;
	GLuint nummaterials = 0;
	
	NSString* path = [[NSBundle mainBundle] pathForResource:@"teapot" ofType:@"qm"];
	
	bool ok = GLLoadMeshFromQM([path UTF8String], &materials, &nummaterials, &mesh);
	
	if( !ok )
	{
		MYERROR("Could not load mesh");
		return;
	}
	
	delete[] materials;

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

	glBindAttribLocation(program, GLDECLUSAGE_POSITION, "my_Position");
	glBindAttribLocation(program, GLDECLUSAGE_NORMAL, "my_Normal");
	glLinkProgram(program);
	
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	
	if( success != GL_TRUE )
	{
		MYERROR("InitScene(): Could not link shader");
		return;
	}

	glUseProgram(program);
	{
		uniform_lightPos = glGetUniformLocation(program, "lightPos");
		uniform_eyePos = glGetUniformLocation(program, "eyePos");
		uniform_matWorld = glGetUniformLocation(program, "matWorld");
		uniform_matViewProj = glGetUniformLocation(program, "matViewProj");
	}
	glUseProgram(0);

	timer = [NSTimer timerWithTimeInterval:(1.0f / 60.0f) target:self selector:@selector(animationTimer:) userInfo:nil repeats:YES];

    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSEventTrackingRunLoopMode];
}

- (void)drawRect:(NSRect)dirtyRect
{
	float lightpos[4]	= { 6, 3, 10, 1 };
	float eye[3]		= { 0, 0, 10 };
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
	
	uint64_t qwtime = mach_absolute_time();
	mach_timebase_info_data_t info;
	mach_timebase_info(&info);
	
	float nano = 1e-9 * ((double)info.numer) / ((double)info.denom);
	float time = (float)qwtime * nano;
	
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	GLMatrixLookAtRH(view, eye, look, up);
	GLMatrixPerspectiveRH(proj, (60.0f * 3.14159f) / 180.f,  screenwidth / screenheight, 0.1f, 100.0f);

	GLMatrixMultiply(viewproj, view, proj);

	GLMatrixRotationAxis(tmp1, fmodf(time * 20.0f, 360.0f) * (3.14152f / 180.0f), 1, 0, 0);
	GLMatrixRotationAxis(tmp2, fmodf(time * 20.0f, 360.0f) * (3.14152f / 180.0f), 0, 1, 0);

	GLMatrixMultiply(world, tmp1, tmp2);

	glUseProgram(program);
	glUniform4fv(uniform_eyePos, 1, eye);
	glUniform4fv(uniform_lightPos, 1, lightpos);
	glUniformMatrix4fv(uniform_matWorld, 1, false, world);
	glUniformMatrix4fv(uniform_matViewProj, 1, false, viewproj);
	{
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				world[12] = (i - 1) * 4;
				world[13] = (j - 1) * 4;

				glUniformMatrix4fv(uniform_matWorld, 1, false, world);
				mesh->DrawSubset(0);
			}
		}
	}
	glUseProgram(0);

	[self.openGLContext flushBuffer];
}

@end
