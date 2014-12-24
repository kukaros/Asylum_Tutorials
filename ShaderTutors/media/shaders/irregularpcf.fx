
#include "commonbrdf.fxh"

sampler basetex : register(s0) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = linear;
};

sampler shadowtex : register(s1) = sampler_state
{
	MinFilter = point;
	MagFilter = point;
	MipFilter = none;

	AddressU = clamp;
	AddressV = clamp;
};

sampler noisetex : register(s2) = sampler_state
{
	MinFilter = point;
	MagFilter = point;
	MipFilter = none;
};

matrix	matWorld;
matrix	matWorldInv;
matrix	matViewProj;
matrix	lightView;
matrix	lightProj;

float4 matSpecular = { 1, 1, 1, 1 };
float4 lightPos;
float3 eyePos;
float2 clipPlanes;
float2 texelSize;
float2 noiseSize;
float2 uv = { 1, 1 };

float4x4 matScale =
{
	0.5f,	0,		0, 0,
	0,		-0.5f,	0, 0,
	0,		0,		1, 0,
	0.5f,	0.5f,	0, 1
};

static const float2 irreg_kernel[8] =
{
	{ -0.072, -0.516 },
	{ -0.105,	0.989 },
	{ 0.949, 0.258 },
	{ -0.966, 0.216 },
	{ 0.784, -0.601 },
	{ 0.139, 0.230 },
	{ -0.816, -0.516 },
	{ 0.529, 0.779 }
};

static const float kernelradius = 2.0f;

void vs_shadowmap(
	in out	float4 pos		: POSITION,
	out		float linearz	: TEXCOORD0)
{
	float4 vpos = mul(mul(pos, matWorld), lightView);

	pos = mul(vpos, lightProj);
	linearz = (vpos.z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);
}

void ps_shadowmap(
	in	float linearz	: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	color = float4(linearz, 0, 0, 1);
}

void vs_irregular(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float4 ltov		: TEXCOORD1,
	out		float3 wnorm	: TEXCOORD2,
	out		float3 ldir		: TEXCOORD3,
	out		float3 vdir		: TEXCOORD4)
{
	float4 wpos = mul(pos, matWorld);
	float4 vpos = mul(wpos, lightView);

	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;
	ldir = lightPos.xyz - wpos.xyz * lightPos.w;
	vdir = eyePos.xyz - wpos.xyz;

	ltov = mul(mul(vpos, lightProj), matScale);
	ltov.z = (vpos.z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);

	pos = mul(wpos, matViewProj);
	tex *= uv;
}

void ps_irregular_screen(
	in	float2 tex		: TEXCOORD0,
	in	float4 ltov		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	in	float3 ldir		: TEXCOORD3,
	in	float3 vdir		: TEXCOORD4,
	in	float2 spos		: VPOS,		// fragment location (window space)
	out	float4 color	: COLOR0)
{
	float2	ptex	= ltov.xy / ltov.w;
	float2	stex	= spos.xy / noiseSize;
	float	d		= ltov.z - 0.002f;
	float	s		= 0;
	float	z;
	float2	irrad	= BRDF_BlinnPhong(wnorm, ldir, vdir, matSpecular.w);
	float4	base	= tex2D(basetex, tex);
	float2	noise	= tex2D(noisetex, stex).xy;
	float2	rotated;

	noise = normalize(noise * 2.0f - 1.0f);

	float2x2 rotmat = { noise.x, -noise.y, noise.y, noise.x };

	for( int i = 0; i < 8; ++i )
	{
		rotated = irreg_kernel[i];
		rotated = mul(rotated, rotmat) * kernelradius;

		z = tex2D(shadowtex, ptex + rotated * texelSize).r;
		s += ((z < d) ? 0.0f : 1.0f);
	}

	s *= 0.125f;
	base.rgb = pow(base.rgb, 2.2f);

	color = (base * irrad.x + irrad.y * matSpecular) * s;
	color.a = 1;
}

void ps_irregular_light(
	in	float2 tex		: TEXCOORD0,
	in	float4 ltov		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	in	float3 ldir		: TEXCOORD3,
	in	float3 vdir		: TEXCOORD4,
	out	float4 color	: COLOR0)
{
	float2	ptex	= ltov.xy / ltov.w;
	float2	stex	= ptex * kernelradius * 15.0f;
	float	d		= ltov.z - 0.002f;
	float	s		= 0;
	float	z;
	float2	irrad	= BRDF_BlinnPhong(wnorm, ldir, vdir, matSpecular.w);
	float4	base	= tex2D(basetex, tex);
	float2	noise	= tex2D(noisetex, stex).xy;
	float2	rotated;

	noise = normalize(noise * 2.0f - 1.0f);

	float2x2 rotmat = { noise.x, -noise.y, noise.y, noise.x };

	for( int i = 0; i < 8; ++i )
	{
		rotated = irreg_kernel[i];
		rotated = mul(rotated, rotmat) * kernelradius;

		z = tex2D(shadowtex, ptex + rotated * texelSize).r;
		s += ((z < d) ? 0.0f : 1.0f);
	}

	s *= 0.125f;
	base.rgb = pow(base.rgb, 2.2f);

	color = (base * irrad.x + irrad.y * matSpecular) * s;
	color.a = 1;
}

technique shadowmap
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_shadowmap();
		pixelshader = compile ps_3_0 ps_shadowmap();
	}
}

technique irregular_screen
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_irregular();
		pixelshader = compile ps_3_0 ps_irregular_screen();
	}
}

technique irregular_light
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_irregular();
		pixelshader = compile ps_3_0 ps_irregular_light();
	}
}
