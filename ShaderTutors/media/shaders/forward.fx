
sampler mytex0 : register(s0) = sampler_state // albedo
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = linear;

	AddressU = wrap;
	AddressV = wrap;
};

sampler mytex2 : register(s2) = sampler_state // shadow
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = none;

	AddressU = clamp;
	AddressV = clamp;
};

matrix matWorld;
matrix matWorldInv;
matrix matViewProj;
matrix lightViewProj;

float4 matDiffuse = { 1, 1, 1, 1 };
float4 lightColor = { 1, 1, 1, 1 };
float4 lightPos;
float4 eyePos;
float4 params = { 1, 1, 1, 1 }; // uv multiplier, <unused>, has texture

float attenuate(float3 ldir)
{
	ldir /= lightPos.w;

	float atten = dot(ldir, ldir);
	float att_s = 15;

	atten = 1.0f / (atten * att_s + 1);
	att_s = 1.0f / (att_s + 1);

	atten = atten - att_s;
	atten /= (1.0f - att_s);

	return saturate(atten);
}

void vs_zonly(
	in out float4 pos : POSITION)
{
	pos = mul(pos, matWorld);
	pos = mul(pos, matViewProj);
}

void vs_forward(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 wnorm	: TEXCOORD1,
	out		float4 wpos		: TEXCOORD2,
	out		float4 cpos		: TEXCOORD3)
{
	wpos = mul(pos, matWorld);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	cpos = mul(wpos, matViewProj);
	pos = cpos;

	tex *= params.xy;
}

void ps_forward_directional(
	in	float2 tex		: TEXCOORD0,
	in	float3 wnorm	: TEXCOORD1,
	in	float4 wpos		: TEXCOORD2,
	in	float4 cpos		: TEXCOORD3,
	out	float4 color	: COLOR0)
{
	float3 n = normalize(wnorm);
	float3 l = normalize(lightPos.xyz);
	float3 v = normalize(eyePos.xyz - wpos.xyz);
	float3 h = normalize(l + v);

	float diffuse = saturate(dot(n, l));
	float specular = saturate(dot(n, h));

	float4 base = tex2D(mytex0, tex);
	base = lerp(matDiffuse, base, params.w);

	specular = pow(specular, 200);

	// shadow term
	float4 lpos = mul(wpos, lightViewProj);
	float2 ltex = (lpos.xy / lpos.w) * float2(0.5f, -0.5f) + 0.5f;

	float2 sd = tex2D(mytex2, ltex).rg;
	float d = length(lightPos.w * lightPos.xyz - wpos.xyz);

	float mean = sd.x;
	float variance = max(sd.y - sd.x * sd.x, 0.0002f);

	float md = mean - d;
	float pmax = variance / (variance + md * md);

	float t = max(d <= mean, pmax);
	float s = ((sd.x < 0.001f) ? 1.0f : t);

	s = saturate(s + 0.1f);

	diffuse *= s;
	specular *= s;

	color.rgb = base.rgb * lightColor.rgb * diffuse + lightColor.rgb * specular;
	color.a = base.a;
}

void ps_forward_point(
	in	float2 tex		: TEXCOORD0,
	in	float3 wnorm	: TEXCOORD1,
	in	float4 wpos		: TEXCOORD2,
	in	float4 cpos		: TEXCOORD2,
	out	float4 color	: COLOR0)
{
	float3 ldir = lightPos.xyz - wpos.xyz;

	float3 n = normalize(wnorm);
	float3 l = normalize(ldir);
	float3 v = normalize(eyePos.xyz - wpos.xyz);
	float3 h = normalize(l + v);

	float diffuse = saturate(dot(n, l));
	float specular = saturate(dot(n, h));
	float atten = attenuate(ldir);

	float4 base = tex2D(mytex0, tex);
	base = lerp(matDiffuse, base, params.w);

	specular = pow(specular, 200);

	// shadow term
	float2 sd = texCUBE(mytex2, -l).rg;
	float d = length(ldir);

	float mean = sd.x;
	float variance = max(sd.y - sd.x * sd.x, 0.0002f);

	float md = mean - d;
	float pmax = variance / (variance + md * md);

	float t = max(d <= mean, pmax);
	float s = ((sd.x < 0.001f) ? 1.0f : t);

	s = saturate(s);
	
	diffuse = diffuse * atten * s;
	specular = specular * atten * s;

	color.rgb = base.rgb * lightColor.rgb * diffuse + lightColor.rgb * specular;
	color.a = base.a;
}

technique zonly
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_zonly();
		pixelshader = null;
	}
}

technique forward_directional
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_forward();
		pixelshader = compile ps_2_0 ps_forward_directional();
	}
}

technique forward_point
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_forward();
		pixelshader = compile ps_2_0 ps_forward_point();
	}
}
