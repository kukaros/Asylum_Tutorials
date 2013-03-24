
sampler mytex0 : register(s0);
sampler mytex1 : register(s1);

matrix matWorld;
matrix matWorldInv;
matrix matViewProj;

float4 matDiffuse = { 1, 1, 1, 1 };
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
}

void ps_gbuffer(
	in	float2 tex		: TEXCOORD0,
	in	float2 zw		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	out	float4 color0	: COLOR0, // albedo
	out	float4 color1	: COLOR1) // normal & depth
{
	float4 base = tex2D(mytex0, tex);

	color0 = lerp(matDiffuse, base, params.w);
	color1 = float4(wnorm, zw.x / zw.y);

	color0.a = 1;
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

void ps_gbuffer_tbn(
	in	float2 tex		: TEXCOORD0,
	in	float2 zw		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	in	float3 wtan		: TEXCOORD3,
	in	float3 wbin		: TEXCOORD4,
	out	float4 color0	: COLOR0, // albedo
	out	float4 color1	: COLOR1) // normal & depth
{
	float3 norm = tex2D(mytex1, tex).rgb * 2 - 1;

	color0 = tex2D(mytex0, tex);
	color0.a = 1;

	float3x3 tbn = { wtan, wbin * params.z, wnorm };

	color1.rgb = normalize(mul(tbn, norm));
	color1.a = zw.x / zw.y;
}

technique gbuffer
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_gbuffer();
		pixelshader = compile ps_2_0 ps_gbuffer();
	}
}

technique gbuffer_tbn
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_gbuffer_tbn();
		pixelshader = compile ps_2_0 ps_gbuffer_tbn();
	}
}
