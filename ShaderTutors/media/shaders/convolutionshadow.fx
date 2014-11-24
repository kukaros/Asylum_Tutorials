
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

sampler sinbasis : register(s2) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = none;

	AddressU = clamp;
	AddressV = clamp;
};

sampler cosbasis : register(s3) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = none;

	AddressU = clamp;
	AddressV = clamp;
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
float2 uv = { 1, 1 };

float4x4 matScale =
{
	0.5f,	0,		0, 0,
	0,		-0.5f,	0, 0,
	0,		0,		1, 0,
	0.5f,	0.5f,	0, 1
};

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

void ps_evalbasis(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR0,
	out	float4 color1	: COLOR1)
{
	float z = tex2D(shadowtex, tex).r;

	// sin(PI * (2k - 1) * z)
	color0.r = sin(3.1415926f * z);
	color0.g = sin(9.4247779f * z);
	color0.b = sin(15.707963f * z);
	color0.a = sin(21.991148f * z);

	color0 = color0 * 0.5f + 0.5f;

	// cos(PI * (2k - 1) * z)
	color1.r = cos(3.1415926f * z);
	color1.g = cos(9.4247779f * z);
	color1.b = cos(15.707963f * z);
	color1.a = cos(21.991148f * z);

	color1 = color1 * 0.5f + 0.5f;
}

void vs_convolution(
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

void ps_convolution(
	in	float2 tex		: TEXCOORD0,
	in	float4 ltov		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	in	float3 ldir		: TEXCOORD3,
	in	float3 vdir		: TEXCOORD4,
	out	float4 color	: COLOR0)
{
	float2	ptex	= ltov.xy / ltov.w;
	float	d		= ltov.z - 0.055f;
	float	s		= 0.5f;
	float	z		= tex2D(shadowtex, ptex).r;
	float2	irrad	= BRDF_BlinnPhong(wnorm, ldir, vdir, matSpecular.w);
	float4	base	= tex2D(basetex, tex);

	float4 sincoeffs = tex2D(sinbasis, ptex) * 2 - 1;
	float4 coscoeffs = tex2D(cosbasis, ptex) * 2 - 1;

	// (2 / ck) * cos(ck * z) * B(z)
	s += 0.6366197f * cos(3.1415926f * d) * sincoeffs.x;
	s += 0.2122065f * cos(9.4247779f * d) * sincoeffs.y;
	s += 0.1273239f * cos(15.707963f * d) * sincoeffs.z;
	s += 0.0909456f * cos(21.991148f * d) * sincoeffs.w;

	// (-2 / ck) * sin(ck * z) * B(z)
	s -= 0.6366197f * sin(3.1415926f * d) * coscoeffs.x;
	s -= 0.2122065f * sin(9.4247779f * d) * coscoeffs.y;
	s -= 0.1273239f * sin(15.707963f * d) * coscoeffs.z;
	s -= 0.0909456f * sin(21.991148f * d) * coscoeffs.w;

	/*
	const float A = 60.0f;
	const float B = 50.0f;

	float p = 8 + A * exp(B * (z - ltov.z));
	*/

	s = saturate((s - 0.06f) * 1.2f);
	s = saturate(pow(s, 8.0f)); // p

	base.rgb = pow(base.rgb, 2.2f);

	color = (base * irrad.x + irrad.y * matSpecular) * s;
	color.a = 1;

	//float test = ((z < ltov.z) ? 0.0f : 1.0f); //p / 70.0f;
	//color = float4(test, test, test, 1);
}

technique shadowmap
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_shadowmap();
		pixelshader = compile ps_3_0 ps_shadowmap();
	}
}

technique evalbasis
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_3_0 ps_evalbasis();
	}
}

technique convolution
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_convolution();
		pixelshader = compile ps_3_0 ps_convolution();
	}
}
