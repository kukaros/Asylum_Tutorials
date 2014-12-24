
#include "commonbrdf.fxh"

#define POS_SCALE_FACTOR	80.0f
#define NEG_SCALE_FACTOR	80.0f

sampler basetex : register(s0) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = linear;
};

sampler shadowtex : register(s1) = sampler_state
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
	float poswarp = exp(POS_SCALE_FACTOR * linearz);
	float negwarp = -1.0f / exp(NEG_SCALE_FACTOR * linearz);

	color = float4(poswarp, poswarp * poswarp, negwarp, negwarp * negwarp);
}

void vs_expvariance(
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

float chebychev(float2 moments, float d)
{
	float mean = moments.x;
	float variance = max(moments.y - moments.x * moments.x, 1e-5f);

	float md = mean - d;
	float pmax = variance / (variance + md * md);
	float bound = max(d <= mean, pmax);

	return bound;
}

void ps_expvariance(
	in	float2 tex		: TEXCOORD0,
	in	float4 ltov		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	in	float3 ldir		: TEXCOORD3,
	in	float3 vdir		: TEXCOORD4,
	out	float4 color	: COLOR0)
{
	float2	ptex	= ltov.xy / ltov.w;
	float	posd	= exp(POS_SCALE_FACTOR * ltov.z);
	float	negd	= -1.0f / exp(NEG_SCALE_FACTOR * ltov.z);
	float	s;
	float4	warps	= tex2D(shadowtex, ptex);
	float2	irrad	= BRDF_BlinnPhong(wnorm, ldir, vdir, matSpecular.w);
	float4	base	= tex2D(basetex, tex);

	float posbound = chebychev(warps.xy, posd);
	float negbound = chebychev(warps.zw, negd);
	
	s = min(posbound, negbound);
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

technique expvariance
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_expvariance();
		pixelshader = compile ps_3_0 ps_expvariance();
	}
}
