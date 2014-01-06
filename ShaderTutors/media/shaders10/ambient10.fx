
uniform Texture2D basetex;

uniform matrix matWorld;
uniform matrix matViewProj;

uniform float4 lightAmbient = { 0, 0, 0, 1 };
uniform float2 uvScale = { 1, 1 };

SamplerState linearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

void vs_ambient(
	in		float3 pos	: POSITION,
	in out	float2 tex	: TEXCOORD0,
	out		float4 opos	: SV_Position)
{
	opos = mul(mul(float4(pos, 1), matWorld), matViewProj);
	tex *= uvScale;
}

void ps_ambient(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: SV_Target)
{
	color = basetex.Sample(linearSampler, tex) * lightAmbient;
}

technique10 ambient
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_ambient()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ps_ambient()));
	}
}
