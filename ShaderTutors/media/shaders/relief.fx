
sampler mytex0 : register(s0) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
};

sampler mytex1 : register(s1) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
};

matrix matWorld;
matrix matWorldInv;
matrix matViewProj;

float4 lightPos = { -10, 10, -10, 1 };
float4 eyePos;
float heightScale = 0.2f;

float rayIntersectHeightfield(float2 dp, float2 ds)
{
	const int linear_search_steps = 15;
	const int binary_search_steps = 15;

	float4 t;
	float size			= 1.0f / linear_search_steps;
	float depth			= 0.0f;		// where we are
	float best_depth	= 1.0f;		// best estimate so far

	// find first point inside heightfield
	for( int i = 0; i < linear_search_steps - 1; ++i )
	{
		depth += size;
		t = tex2D(mytex1, dp + ds * depth);

		if( best_depth > 0.996f )
		{
			if( depth >= (1.0f - t.w) )
				best_depth = depth;
		}
	}

	depth = best_depth;

	// look for exact intersection
	for( int i = 0; i < binary_search_steps; ++i )
	{
		size *= 0.5;
		t = tex2D(mytex1, dp + ds * depth);

		if( depth >= (1.0f - t.w) )
		{
			best_depth = depth;
			depth -= 2 * size;
		}

		depth += size;
	}

	return best_depth;
}

void vs_main(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in		float3 tang		: TANGENT,
	in		float3 bin		: BINORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 vdir		: TEXCOORD1,
	out		float3 wtan		: TEXCOORD2,
	out		float3 wbin		: TEXCOORD3,
	out		float3 wnorm	: TEXCOORD4)
{
	wnorm = normalize(mul(matWorldInv, float4(norm, 0)).xyz);
	wtan = normalize(mul(float4(tang, 0), matWorld).xyz);
	wbin = normalize(mul(float4(bin, 0), matWorld).xyz);

	pos = mul(pos, matWorld);
	vdir = pos.xyz - eyePos.xyz;

	pos = mul(pos, matViewProj);
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 vdir		: TEXCOORD1,
	in	float3 wtan		: TEXCOORD2,
	in	float3 wbin		: TEXCOORD3,
	in	float3 wnorm	: TEXCOORD4,
	out	float4 color	: COLOR0)
{
	float3 p = vdir;
	float3 v = normalize(vdir);

	float3x3 tbn = { wtan, -wbin, wnorm };
	float3 s = normalize(mul(tbn, v));

	float2 dp = tex;
	float2 ds = float2(-s.x, s.y) * heightScale / s.z;

	// find intersection
	float d = rayIntersectHeightfield(dp, ds);

	tex = dp + ds * d;
	p += v * d * s.z;

	// put light vector to tangent space
	float3 l = normalize(lightPos.xyz - p);
	l = normalize(mul(tbn, l));

	float3 n = tex2D(mytex1, tex) * 2 - 1;
	float3 h = normalize(l - s);

	// irradiance
	float diffuse = saturate(dot(n, l));
	float specular = saturate(dot(n, h));

	diffuse = diffuse * 0.8f + 0.2f;
	specular = pow(specular, 80);

#if 1 // soft shadows
	l.xy *= heightScale;
	l.y *= -1;

	float sh0 = tex2D(mytex1, tex).a;
	float softy = 0.58f;

	// shi = 1 * (Db - Ds) * (1 / Dlb')
	float shA = (tex2D(mytex1, tex + l.xy * 0.88f).a - sh0) *  1 * softy;
	float sh9 = (tex2D(mytex1, tex + l.xy * 0.77f).a - sh0) *  2 * softy;
	float sh8 = (tex2D(mytex1, tex + l.xy * 0.66f).a - sh0) *  4 * softy;
	float sh7 = (tex2D(mytex1, tex + l.xy * 0.55f).a - sh0) *  6 * softy;
	float sh6 = (tex2D(mytex1, tex + l.xy * 0.44f).a - sh0) *  8 * softy;
	float sh5 = (tex2D(mytex1, tex + l.xy * 0.33f).a - sh0) * 10 * softy;
	float sh4 = (tex2D(mytex1, tex + l.xy * 0.22f).a - sh0) * 12 * softy;

	float shadow = 1 - max(max(max(max(max(max(shA, sh9), sh8), sh7), sh6), sh5), sh4);
	shadow = shadow * 0.3 + 0.7;

	diffuse *= shadow;
	specular *= shadow;
#endif

	// final color
	color = tex2D(mytex0, tex) * diffuse + specular;
	color.a = 1;
}

technique relief
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_main();
		pixelshader = compile ps_3_0 ps_main();
	}
}
