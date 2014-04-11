
sampler mytex0 : register(s0) = sampler_state // albedo
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = linear;

	AddressU = wrap;
	AddressV = wrap;
};

sampler mytex1 : register(s1) = sampler_state // normalmap
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

float attenuate(in float3 ldir)
{
	float atten = dot(ldir, ldir) / (lightPos.w * lightPos.w);
	atten = 1.0 / (atten * 15.0 + 1.0);
	atten = (atten - 0.0625) * 1.066666;
	return saturate(atten);
}

float2 diffuseWithSpecular(
	in float3 ldir,
	in float3 vdir,
	in float3 wnorm,
	in float power)
{
	float3 n = normalize(wnorm);
	float3 l = normalize(ldir);
	float3 v = normalize(vdir);
	float3 h = normalize(l + v);
	float2 ret;

	ret.x = saturate(dot(l, n));
	ret.y = pow(saturate(dot(h, n)), power);

	return ret;
}

float4 pointLight(
	in float2 tex,
	in float3 ldir,
	in float3 vdir,
	in float3 wldir,
	in float3 wnorm)
{
	float2 diffspec = diffuseWithSpecular(ldir, vdir, wnorm, 80.0f);
	float4 base = tex2D(mytex0, tex);
	float atten = attenuate(wldir);

	base.rgb = pow(base.rgb, 2.2f);

	float4 diff = base * lightColor;
	float4 ret = (diff + diffspec.y) * diffspec.x * atten;

	ret.a = base.a;
	return ret;
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

void vs_forward_point_tbn(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in		float3 tang		: TANGENT,
	in		float3 bin		: BINORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 wldir	: TEXCOORD5,
	out		float3 ldir		: TEXCOORD6,
	out		float3 vdir		: TEXCOORD7)
{
	float4 wpos = mul(float4((pos).xyz, 1.0f), matWorld);
	wldir = lightPos.xyz - wpos.xyz;

	float3 v = eyePos.xyz - wpos.xyz;
	float3 wnorm = normalize(mul(matWorldInv, float4(norm, 0.0f)).xyz);
	float3 wtan = normalize(mul(float4(tang, 0.0f), matWorld).xyz);
	float3 wbin = normalize(mul(float4(bin, 0.0f), matWorld).xyz);
	
	ldir.x = dot(wldir, wtan);
	ldir.y = dot(wldir, wbin);
	ldir.z = dot(wldir, wnorm);
	
	vdir.x = dot(v, wtan);
	vdir.y = dot(v, wbin);
	vdir.z = dot(v, wnorm);

	pos = mul(wpos, matViewProj);
}

void ps_forward_point_tbn(
	in	float2 tex		: TEXCOORD0,
	in	float3 wldir	: TEXCOORD5,
	in	float3 ldir		: TEXCOORD6,
	in	float3 vdir		: TEXCOORD7,
	out	float4 color	: COLOR)
{
	float3 norm;

	norm = (tex2D(mytex1, tex) * 2.0f - 1.0f).xyz;
	color = pointLight(tex, ldir, vdir, wldir, norm);
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

technique forward_point_tbn
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_forward_point_tbn();
		pixelshader = compile ps_2_0 ps_forward_point_tbn();
	}
}
