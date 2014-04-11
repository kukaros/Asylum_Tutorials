
matrix matWorld;
matrix matWorldSky;
matrix matViewProj;

float4 eyePos;
float degamma = 1.0f;

samplerCUBE mytex0 : register(s0);

void vs_sky(
	in out	float4 pos	: POSITION,
	out		float3 view	: TEXCOORD0)
{
	pos = mul(pos, matWorld);
	view = pos.xyz - eyePos.xyz;

	view = mul(float4(view, 1), matWorldSky).rgb;

	pos = mul(pos, matViewProj);
}

void ps_sky(
	in	float3 view		: TEXCOORD0,
	out	float4 color	: COLOR)
{
	float4 base = texCUBE(mytex0, view);

	color = float4(pow(base.rgb, degamma), 1);
}

technique sky
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_sky();
		pixelshader = compile ps_2_0 ps_sky();
	}
}
