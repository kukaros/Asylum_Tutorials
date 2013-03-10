
sampler mytex0 : register(s0);

matrix matWorld;
matrix matWorldInv;
matrix matViewProj;

void vs_main(
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

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float2 zw		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	out	float4 color0	: COLOR0, // albedo
	out	float4 color1	: COLOR1) // normal & depth
{
	color0 = tex2D(mytex0, tex);
	color1 = float4(wnorm, zw.x / zw.y);

	color0.a = 1;
}

technique wbuffer
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_main();
		pixelshader = compile ps_2_0 ps_main();
	}
}
