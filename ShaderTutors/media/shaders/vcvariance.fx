
sampler mytex0 : register(s0);
sampler mytex1 : register(s1);

samplerCUBE mytex2 : register(s2);

matrix matWorld;
matrix matViewProj;
matrix lightVP;

float4 lightPos;
float2 uv = { 1, 1 };

void vs_main(
	in out	float4 pos	: POSITION,
	in out	float2 tex	: TEXCOORD0,
	out		float3 wpos	: TEXCOORD1)
{
	pos = mul(pos, matWorld);
	wpos = pos.xyz;

	pos = mul(pos, matViewProj);
	tex *= uv;
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 wpos		: TEXCOORD1,
	out	float4 color	: COLOR0)
{
	float3 ldir = wpos - lightPos.xyz;
	float4 tx = texCUBE(mytex2, ldir);
	float2 sd = tex2D(mytex1, tx.xy).rg;

	float s;
	float d = length(ldir);

	float mean = sd.x;
	float variance = max(sd.y - sd.x * sd.x, 0.0002f);

	float md = mean - d;
	float pmax = variance / (variance + md * md);

	s = max(d <= mean, pmax);
	s = saturate(s + 0.3f);

	color = tex2D(mytex0, tex) * s;
	color.a = 1;
}

technique variance
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_main();
		pixelshader = compile ps_2_0 ps_main();
	}
}
