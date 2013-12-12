
sampler mytex0 : register(s0);
sampler mytex1 : register(s1);

matrix matWorld;
matrix matWorldInv;
matrix matViewProj;

float4 matDiffuse = { 1, 1, 1, 1 };
float4 matShininess = { 80.0f, 1, 1, 1 };
float4 params = { 1, 1, 1, 1 }; // uv multiplier, binormal sign, has texture

void vs_gbuffer(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float2 zw		: TEXCOORD1,
	out		float3 wnorm	: TEXCOORD2)
{
	pos = mul(pos, matWorld);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	pos = mul(pos, matViewProj);
	zw = pos.zw;

	tex *= params.xy;
}

void vs_gbuffer_tbn(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in		float3 tang		: TANGENT,
	in		float3 bin		: BINORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float2 zw		: TEXCOORD1,
	out		float3 wnorm	: TEXCOORD2,
	out		float3 wtan		: TEXCOORD3,
	out		float3 wbin		: TEXCOORD4)
{
	pos = mul(pos, matWorld);

	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;
	wtan = mul(float4(tang, 0), matWorld).xyz;
	wbin = mul(float4(bin, 0), matWorld).xyz;

	pos = mul(pos, matViewProj);
	zw = pos.zw;

	tex *= params.xy;
}

void ps_gbuffer_ds(
	in	float2 tex		: TEXCOORD0,
	in	float2 zw		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	out	float4 color0	: COLOR0, // albedo
	out	float4 color1	: COLOR1, // normals
	out	float4 color2	: COLOR2) // depth
{
	float4 base = tex2D(mytex0, tex);

	color0 = lerp(matDiffuse, base, params.w);
	color1 = float4(wnorm, 1);
	color2 = float4(zw.x / zw.y, 0, 0, 1);

	color0.a = 1;
}

void ps_gbuffer_tbn_ds(
	in	float2 tex		: TEXCOORD0,
	in	float2 zw		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	in	float3 wtan		: TEXCOORD3,
	in	float3 wbin		: TEXCOORD4,
	out	float4 color0	: COLOR0, // albedo
	out	float4 color1	: COLOR1, // normals
	out	float4 color2	: COLOR2) // depth
{
	float3 norm = tex2D(mytex1, tex).rgb * 2 - 1;
	float4 base = tex2D(mytex0, tex);

	color0 = lerp(matDiffuse, base, params.w);
	color0.a = 1;

	float3x3 tbn = { wtan, wbin * params.z, wnorm };

	color1.rgb = normalize(mul(tbn, norm));
	color1.a = 1;

	color2 = float4(zw.x / zw.y, 0, 0, 1);
}

void ps_gbuffer_dl(
	in	float2 tex		: TEXCOORD0,
	in	float2 zw		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	out	float4 color0	: COLOR0, // normal & shininess
	out	float4 color1	: COLOR1) // depth
{
	float4 spec = tex2D(mytex0, tex);

	spec.r = lerp(matShininess.x, spec.r, params.w);

	color0 = float4(wnorm, spec.r);
	color1 = zw.x / zw.y;
}

void ps_gbuffer_tbn_dl(
	in	float2 tex		: TEXCOORD0,
	in	float2 zw		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	in	float3 wtan		: TEXCOORD3,
	in	float3 wbin		: TEXCOORD4,
	out	float4 color0	: COLOR0, // normal & shininess
	out	float4 color1	: COLOR1) // depth
{
	float4 spec = tex2D(mytex0, tex);
	float3 norm = tex2D(mytex1, tex).rgb * 2 - 1;

	spec.r = lerp(matShininess.x, spec.r, params.w);

	float3x3 tbn = { wtan, wbin * params.z, wnorm };

	color0.rgb = normalize(mul(tbn, norm));
	color0.a = spec.r;

	color1 = zw.x / zw.y;
}

technique gbuffer_dshading
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_gbuffer();
		pixelshader = compile ps_2_0 ps_gbuffer_ds();
	}
}

technique gbuffer_tbn_dshading
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_gbuffer_tbn();
		pixelshader = compile ps_2_0 ps_gbuffer_tbn_ds();
	}
}

technique gbuffer_dlighting
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_gbuffer();
		pixelshader = compile ps_2_0 ps_gbuffer_dl();
	}
}

technique gbuffer_tbn_dlighting
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_gbuffer_tbn();
		pixelshader = compile ps_2_0 ps_gbuffer_tbn_dl();
	}
}
