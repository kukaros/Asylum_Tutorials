
uniform Texture2D basetex;

uniform matrix matWorld;
uniform matrix matViewProj;

SamplerState linearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

void vs_zonly(
	in	float3 pos	: POSITION,
	out	float4 opos	: SV_Position)
{
	opos = mul(mul(float4(pos, 1), matWorld), matViewProj);
}

void ps_zonly(
	out float4 color : SV_Target)
{
	color = float4(0, 0, 0, 1);
}

technique10 zonly
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_zonly()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ps_zonly()));
	}
}
