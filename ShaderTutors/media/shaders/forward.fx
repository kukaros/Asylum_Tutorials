
sampler mytex0 : register(s0);

matrix matWorld;
matrix matWorldInv;
matrix matViewProj;

float4 lightColor;
float4 lightPos;
float4 eyePos;

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
	out		float4 wpos		: TEXCOORD2)
{
	wpos = mul(pos, matWorld);
	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;

	pos = mul(wpos, matViewProj);
} 

void ps_forward(
	in	float2 tex		: TEXCOORD0,
	in	float3 wnorm	: TEXCOORD1,
	in	float4 wpos		: TEXCOORD2,
	out	float4 color	: COLOR0)
{
	float3 l = normalize(lightPos.xyz - wpos.xyz);
	float3 v = normalize(eyePos.xyz - wpos.xyz);
	float3 h = normalize(l + v);
	float3 n = normalize(wnorm);

	float diffuse	= saturate(dot(n, l));
	float specular	= saturate(dot(n, h));
	float dist		= length(lightPos.xyz - wpos.xyz);
	float atten		= saturate(1.0f - (dist / lightPos.w));

	specular = pow(specular, 200);

	float4 base = tex2D(mytex0, tex);
	float4 lightd = lightColor * diffuse * atten;
	float4 lights = lightColor * specular * atten;

	base.rgb = pow(base.rgb, 2.2f);

	color = base * lightd + lights;
	color.a = 1;
}

technique zonly
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_zonly();
		pixelshader = null;
	}
}

technique forward
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_forward();
		pixelshader = compile ps_2_0 ps_forward();
	}
}
