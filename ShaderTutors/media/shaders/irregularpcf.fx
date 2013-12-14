
sampler mytex0 : register(s0);
sampler mytex1 : register(s1);
sampler mytex2 : register(s2);

matrix	matWorld;
matrix	matViewProj;
matrix	lightVP;

float4	lightPos;
float2	noisesize;
float2	texelsize;
float	kernelradius = 2.0f;

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

void vs_screen(
	in out	float4 pos	: POSITION,
	in out	float2 tex	: TEXCOORD0,
	out		float4 ltov	: TEXCOORD1,
	out		float3 wpos	: TEXCOORD2)
{
	pos = mul(pos, matWorld);
	wpos = pos.xyz;

	ltov = mul(mul(pos, lightVP), matScale);
	pos = mul(pos, matViewProj);

	tex *= float2(2, 2);
}

void ps_screen(
	in	float2 tex		: TEXCOORD0,
	in	float4 ltov		: TEXCOORD1,
	in	float3 wpos		: TEXCOORD2,
	in	float2 spos		: VPOS,		// fragment location (window space)
	out	float4 color	: COLOR0)
{
	// TODO: distance is not really good for directional lights
	float d = length(lightPos.xyz - wpos);
	float sd, s = 0, t;

	float2 ptex = ltov.xy / ltov.w;
	float2 stex = spos.xy / noisesize;
	float2 noise;
	float2 rotated;

	noise = tex2D(mytex2, stex);
	noise = normalize(noise * 2.0f - 1.0f);

	float2x2 rotmat = { noise.x, -noise.y, noise.y, noise.x };

	for( int i = 0; i < 8; ++i )
	{
		rotated = irreg_kernel[i];
		rotated = mul(rotated, rotmat) * kernelradius;

		sd = tex2D(mytex1, ptex + rotated * texelsize).r;

		t = (d <= sd);
		s += ((sd < 0.01f) ? 1 : t);
	}

	s = saturate(s * 0.125f + 0.3f); // simulate ambient light here

	color = tex2D(mytex0, tex) * s;
	color.a = 1;
}

void ps_light(
	in	float2 tex		: TEXCOORD0,
	in	float4 ltov		: TEXCOORD1,
	in	float3 wpos		: TEXCOORD2,
	out	float4 color	: COLOR0)
{
	float d = length(lightPos.xyz - wpos);
	float sd, s = 0, t;

	float2 ptex = ltov.xy / ltov.w;
	float2 stex = ptex * kernelradius * 15.0f;
	float2 noise;
	float2 rotated;

	noise = tex2D(mytex2, stex);
	noise = normalize(noise * 2.0f - 1.0f);

	float2x2 rotmat = { noise.x, -noise.y, noise.y, noise.x };

	for( int i = 0; i < 8; ++i )
	{
		rotated = irreg_kernel[i];
		rotated = mul(rotated, rotmat) * kernelradius;

		sd = tex2D(mytex1, ptex + rotated * texelsize).r;

		t = (d <= sd);
		s += ((sd < 0.01f) ? 1 : t);
	}

	s = saturate(s * 0.125f + 0.3f); // simulate ambient light here

	color = tex2D(mytex0, tex) * s;
	color.a = 1;
}

technique irregular_screen
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_screen();
		pixelshader = compile ps_3_0 ps_screen();
	}
}

technique irregular_light
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_screen();
		pixelshader = compile ps_3_0 ps_light();
	}
}
