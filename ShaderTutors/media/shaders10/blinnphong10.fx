
uniform Texture2D basetex;

uniform matrix matWorld;
uniform matrix matWorldInv;
uniform matrix matViewProj;

uniform float4 lightPos = { -10, 10, -10, 1 };
uniform float4 eyePos;
uniform float2 uvScale;

SamplerState linearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

void vs_main(
	in		float3 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 wnorm	: TEXCOORD1,
	out		float3 ldir		: TEXCOORD2,
	out		float3 vdir		: TEXCOORD3,
	out		float4 opos		: SV_Position)
{
	float4 wpos = mul(float4(pos, 1), matWorld);

	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;
	ldir = lightPos.xyz - wpos.xyz;
	vdir = eyePos.xyz - wpos.xyz;

	opos = mul(wpos, matViewProj);
	tex *= uvScale;
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 wnorm	: TEXCOORD1,
	in	float3 ldir		: TEXCOORD2,
	in	float3 vdir		: TEXCOORD3,
	out float4 color	: SV_Target)
{
	float3 l = normalize(ldir);
	float3 v = normalize(vdir);
	float3 h = normalize(l + v);
	float3 n = normalize(wnorm);

	float diffuse = saturate(dot(n, l));
	float specular = saturate(dot(n, h));

	specular = pow(specular, 60);

	color = basetex.Sample(linearSampler, tex) * diffuse + specular;
	color.a = 1;
}

technique10 specular
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_main()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ps_main()));
	}
}
