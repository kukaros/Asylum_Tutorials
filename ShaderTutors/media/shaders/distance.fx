
matrix matWorld;
matrix matViewProj;

float4 lightPos;
float3 texelSize;

sampler mytex0 : register(s0);

static const float3 offsets[] =
{
	{ -2, -2, -2 },
	{ -1, -1, -1 },
	{ 0, 0, 0 },
	{ 1, 1, 1 },
	{ 2, 2, 2 }
};

void vs_point(
	in out	float4	pos		: POSITION,
	out		float3	wpos	: TEXCOORD0)
{
	pos = mul(pos, matWorld);
	wpos = pos.xyz;

	pos = mul(pos, matViewProj);
}

void ps_point(
	in	float3	wpos	: TEXCOORD0,
	out	float4	color	: COLOR0)
{
	float d = length(lightPos.xyz - wpos);
	color = float4(d, d * d, 0, 1);
}

void vs_directional(
	in out	float4	pos	: POSITION,
	out		float4	wpos: TEXCOORD0)
{
	wpos = mul(pos, matWorld);
	pos = mul(wpos, matViewProj);
}

void ps_directional(
	in	float4	wpos	: TEXCOORD0,
	out	float4	color	: COLOR0)
{
	float d = length(lightPos.w * lightPos.xyz - wpos.xyz);
	color = float4(d, d * d, 0, 1);
}

void ps_blur5x5(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR)
{
	float4 s = 0;

	s += tex2D(mytex0, tex + offsets[0].xy * texelSize.xy);
	s += tex2D(mytex0, tex + offsets[1].xy * texelSize.xy);
	s += tex2D(mytex0, tex + offsets[2].xy * texelSize.xy);
	s += tex2D(mytex0, tex + offsets[3].xy * texelSize.xy);
	s += tex2D(mytex0, tex + offsets[4].xy * texelSize.xy);

	s *= (1.0f / 5.0f);
	color = s;
}

void vs_blurcube5x5(
	in out	float4	pos		: POSITION,
	out		float3	cdir	: TEXCOORD0)
{
	cdir = pos;
	pos = mul(pos, matViewProj);
}

void ps_blurcube5x5(
	in	float3 cdir		: TEXCOORD0,
	out	float4 color	: COLOR)
{
	float4 s = 0;

	s = texCUBE(mytex0, cdir);

	// texelSize must be multiplied by 2
	s += texCUBE(mytex0, cdir + offsets[0] * texelSize);
	s += texCUBE(mytex0, cdir + offsets[1] * texelSize);
	s += texCUBE(mytex0, cdir + offsets[2] * texelSize);
	s += texCUBE(mytex0, cdir + offsets[3] * texelSize);
	s += texCUBE(mytex0, cdir + offsets[4] * texelSize);

	s *= (1.0f / 5.0f);
	color = s;
}

technique distance_point
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_point();
		pixelshader = compile ps_2_0 ps_point();
	}
}

technique distance_directional
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_directional();
		pixelshader = compile ps_2_0 ps_directional();
	}
}

technique blur5x5
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_3_0 ps_blur5x5();
	}
}

technique blurcube5x5
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_blurcube5x5();
		pixelshader = compile ps_3_0 ps_blurcube5x5();
	}
}
