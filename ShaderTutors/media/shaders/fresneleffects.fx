
#define USE_SCHLICK		1

matrix matWorld;
matrix matWorldInv;
matrix matViewProj;

float4 eyePos;
float4 matDiffuse = { 0.5f, 0.75f, 0.5f, 1 };
float2 refractiveindices = { 1.000293f, 1.53f };

samplerCUBE mytex0 : register(s0) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = linear;
};

sampler2D mytex1 : register(s1) = sampler_state
{
	MinFilter = point;
	MagFilter = point;

	AddressU = border;
	AddressV = border;
};

sampler2D mytex2 : register(s2) = sampler_state
{
	MinFilter = point;
	MagFilter = point;

	AddressU = border;
	AddressV = border;
};

// =======================================================================
//
// Geometry buffers for double refraction
//
// =======================================================================

void vs_geombuffer(
	in		float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	out		float4 opos		: POSITION,
	out		float3 wnorm	: TEXCOORD0,
	out		float3 wpos		: TEXCOORD1)
{
	float4 p = mul(pos, matWorld);

	wpos = p.xyz;
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	opos = mul(p, matViewProj);
}

void ps_geombuffer(
	in		float3 wnorm	: TEXCOORD0,
	in		float3 wpos		: TEXCOORD1,
	out		float4 color0	: COLOR0,
	out		float4 color1	: COLOR1)
{
	float3 n = normalize(wnorm);

	color0 = float4(n * 0.5f + 0.5f, 1);
	color1 = float4(wpos, 1);
}

// =======================================================================
//
// Single refraction
//
// =======================================================================

void vs_singlerefract(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 incray	: TEXCOORD1,
	out		float3 wnorm	: TEXCOORD2)
{
	pos = mul(pos, matWorld);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	incray = pos.xyz - eyePos.xyz;
	pos = mul(pos, matViewProj);
}

void ps_singlerefract(
	in	float2 tex		: TEXCOORD0,
	in	float3 incray	: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	out	float4 color	: COLOR0,
	uniform const bool transparent)
{
	float n1 = refractiveindices.x;
	float n2 = refractiveindices.y;
	float ratio = n1 / n2;
	float R;

	float3 n = normalize(wnorm);
	float3 i = normalize(incray);
	float3 reflray = reflect(i, n);
	float3 refrray = refract(i, n, ratio);
	float4 reflcol = texCUBE(mytex0, reflray);

#if USE_SCHLICK
	float R0 = (n1 - n2) / (n1 + n2);

	R0 *= R0;
	R = saturate(R0 + (1 - R0) * pow(1 + dot(i, n), 5));
#else
	float cosA = dot(reflray, n);
	float cosG = -dot(refrray, n);

	float Rs = (n1 * cosA - n2 * cosG) / (n1 * cosA + n2 * cosG);
	float Rp = (n1 * cosG - n2 * cosA) / (n1 * cosG + n2 * cosA);
	
	R = (Rs * Rs + Rp * Rp) * 0.5f;
#endif

	if( transparent )
	{
		float4 refrcol = texCUBE(mytex0, refrray);
		color = lerp(refrcol * matDiffuse, reflcol, R);
	}
	else
		color = lerp(matDiffuse, reflcol, R);
}

// =======================================================================
//
// Chromatic dispersion
//
// =======================================================================

void ps_disperse(
	in	float2 tex		: TEXCOORD0,
	in	float3 incray	: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	out	float4 color	: COLOR0)
{
	float n1 = refractiveindices.x;
	float n2 = refractiveindices.y;
	float n3 = refractiveindices.y + 0.1f;
	float n4 = refractiveindices.y + 0.25f;

	float3 n = normalize(wnorm);
	float3 i = normalize(incray);

	float3 reflray = reflect(i, n);
	float3 refrray1 = refract(i, n, n1 / n2);
	float3 refrray2 = refract(i, n, n1 / n3);
	float3 refrray3 = refract(i, n, n1 / n4);

	float4 reflcol = texCUBE(mytex0, reflray);
	float4 refrcol1 = texCUBE(mytex0, refrray1);
	float4 refrcol2 = texCUBE(mytex0, refrray2);
	float4 refrcol3 = texCUBE(mytex0, refrray3);

	float R0r = (n1 - n2) / (n1 + n2);
	float R0g = (n1 - n3) / (n1 + n3);
	float R0b = (n1 - n4) / (n1 + n4);

	R0r *= R0r;
	R0g *= R0g;
	R0b *= R0b;

	float p = pow(1 + dot(i, n), 5);
	float Rr = saturate(R0r + (1 - R0r) * p);
	float Rg = saturate(R0g + (1 - R0g) * p);
	float Rb = saturate(R0b + (1 - R0b) * p);

	color.r = lerp(refrcol1.r, reflcol.r, Rr);
	color.g = lerp(refrcol2.g, reflcol.g, Rg);
	color.b = lerp(refrcol3.b, reflcol.b, Rb);
	color.a = 1;
}

// =======================================================================
//
// Double refraction with Newton - Rhapson iteration
//
// =======================================================================

void NewtonRhapson2D(
	out		float3		hit,
	out		float3		norm,
	in		float3		start,
	in		float3		dir,
	in		float4x4	viewproj,
	in		sampler2D	normalmap,
	in		sampler2D	positionsmap)
{
	float d = 1.0;
	float3 p0 = start;
	float3 r = normalize(dir);
	float3 p1;
	float4 cp1;
	float2 tex;

	// iter1
	p1 = p0 + d * r;
	cp1 = mul(float4(p1, 1), viewproj);

	tex = (cp1.xy / cp1.w) * float2(0.5, -0.5) + 0.5;
	p1 = tex2Dlod(positionsmap, float4(tex, 0, 0)).xyz;

	d = length(p1 - p0);

	// iter2
	p1 = p0 + d * r;
	cp1 = mul(float4(p1, 1), viewproj);

	tex = (cp1.xy / cp1.w) * float2(0.5, -0.5) + 0.5;
	p1 = tex2Dlod(positionsmap, float4(tex, 0, 0)).xyz;

	d = length(p1 - p0);

	// iter3
	p1 = p0 + d * r;
	cp1 = mul(float4(p1, 1), viewproj);

	tex = (cp1.xy / cp1.w) * float2(0.5, -0.5) + 0.5;
	p1 = tex2Dlod(positionsmap, float4(tex, 0, 0)).xyz;

	d = length(p1 - p0);

	// return
	hit = p1;
	norm = normalize(tex2Dlod(normalmap, float4(tex, 0, 0)).xyz * 2 - 1);
}

void vs_doublerefract(
	in	float4 pos		: POSITION,
	in	float3 norm		: NORMAL,
	out	float4 opos		: POSITION,
	out	float3 wpos		: TEXCOORD0,
	out	float3 wnorm	: TEXCOORD1)
{
	float4 p = mul(pos, matWorld);

	wpos = p.xyz;
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	opos = mul(p, matViewProj);
}

void ps_doublerefract(
	in	float3 wpos		: TEXCOORD0,
	in	float3 wnorm	: TEXCOORD1,
	out	float4 color	: COLOR)
{
	// TODO: can be done on CPU
	float ratio		= refractiveindices.x / refractiveindices.y;
	float invratio	= refractiveindices.y / refractiveindices.x;
	float R0		= (refractiveindices.x - refractiveindices.y) / (refractiveindices.x + refractiveindices.y);
	float R0inv		= (refractiveindices.y - refractiveindices.x) / (refractiveindices.x + refractiveindices.y);

	R0 *= R0;
	R0inv *= R0inv;

	float4 refr, refl;
	float3 n0 = normalize(wnorm);
	float3 x0 = wpos;
	float3 v0 = normalize(x0 - eyePos);
	float3 x1, n1, x2 = 0, n2;
	float3 v1, v2;

	float fresnel;
	float power = 1;

	refr = float4(1, 0, 0, 1);

	// first bounce occurs when ray enters object
	v1 = refract(v0, n0, ratio);
	v2 = reflect(v0, n0);

	fresnel = R0 + (1 - R0) * pow(1 + dot(n0, v0), 5); 
	refl = texCUBE(mytex0, v2) * fresnel;

	if( dot(v1, v1) > 0 )
	{
		power = (1 - fresnel);

		// and hits a point on its inside
		NewtonRhapson2D(x1, n1, x0, v1, matViewProj, mytex1, mytex2);

		// second bounce when ray leaves object
		v2 = refract(v1, -n1, invratio);
		v1 = reflect(v1, -n1);

		if( dot(v2, v2) > 0 )
			refr = texCUBE(mytex0, v2) * power; // hit sky
		else
			refr = texCUBE(mytex0, v1) * power; // TIR
	}

	color = (refr * matDiffuse + refl);
	color.a = 1;
}

// =======================================================================
//
// Techniques
//
// =======================================================================

technique reflection
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_singlerefract();
		pixelshader = compile ps_2_0 ps_singlerefract(false);
	}
}

technique singlerefraction
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_singlerefract();
		pixelshader = compile ps_2_0 ps_singlerefract(true);
	}
}

technique dispersion
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_singlerefract();
		pixelshader = compile ps_3_0 ps_disperse();
	}
}

technique doublerefraction
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_doublerefract();
		pixelshader = compile ps_3_0 ps_doublerefract();
	}
}

technique geombuffer
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_geombuffer();
		pixelshader = compile ps_3_0 ps_geombuffer();
	}
}
