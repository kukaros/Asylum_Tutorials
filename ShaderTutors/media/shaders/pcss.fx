
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
float2 lightRadius;		// in light space
float2 uv = { 1, 1 };

static const float4x4 matScale =
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

void vs_pcss(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float4 ltov		: TEXCOORD1,
	out		float3 wnorm	: TEXCOORD2,
	out		float3 ldir		: TEXCOORD3,
	out		float3 vdir		: TEXCOORD4,
	out		float4 vpos		: TEXCOORD5)
{
	float4 wpos = mul(pos, matWorld);

	vpos = mul(wpos, lightView);

	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;
	ldir = lightPos.xyz - wpos.xyz * lightPos.w;
	vdir = eyePos.xyz - wpos.xyz;

	ltov = mul(mul(vpos, lightProj), matScale);
	ltov.z = (vpos.z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);

	pos = mul(wpos, matViewProj);
	tex *= uv;
}

float2 findAverageBlockerDepth(float d, float zeye, float2 uv)
{
	float2 searchregion = lightRadius * (zeye - clipPlanes.x) / zeye;
	float2 stepsize = searchregion / 3.0f;
	float2 ret = { 0, 0 };
	float z;

	for( int i = -3; i <= 3; ++i )
	{
		for( int j = -3; j <= 3; ++j )
		{
			z = tex2D(shadowtex, uv + float2(i, j) * stepsize).r;

			if( z < d )
			{
				ret.x += z;
				++ret.y;
			}
		}
	}

	ret.x /= ret.y;
	return ret;
}

void ps_pcss(
	in	float2 tex		: TEXCOORD0,
	in	float4 ltov		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	in	float3 ldir		: TEXCOORD3,
	in	float3 vdir		: TEXCOORD4,
	in	float4 vpos		: TEXCOORD5,
	out	float4 color	: COLOR0)
{
	float2	ptex	= ltov.xy / ltov.w;
	float	d		= ltov.z - 0.002f;
	float	s		= 1;
	float	z;
	float2	irrad	= BRDF_BlinnPhong(wnorm, ldir, vdir, matSpecular.w);
	float4	base	= tex2D(basetex, tex);

	float2 avgdepth = findAverageBlockerDepth(d, vpos.z, ptex);

	if( avgdepth.y > 0.0f )
	{
		float avgdepthls	= clipPlanes.x + avgdepth.x * (clipPlanes.y - clipPlanes.x);
		float2 penumbra		= lightRadius * (vpos.z - avgdepthls) / avgdepthls;
		float2 filterradius	= penumbra * clipPlanes.x / vpos.z;
		float2 stepuv		= (filterradius / 2);

		s = 0;

		for( int i = -2; i < 3; ++i )
		{
			for( int j = -2; j < 3; ++j )
			{
				z = tex2D(shadowtex, ptex + float2(i, j) * stepuv).r;
				s += ((z < d) ? 0.0f : 1.0f);
			}
		}

		s /= 25.0f;
	}

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

technique pcss
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_pcss();
		pixelshader = compile ps_3_0 ps_pcss();
	}
}
