
sampler mytex0 : register(s0) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
};

sampler mytex1 : register(s1) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
};

matrix matWorld;
matrix matWorldInv;
matrix matViewProj;

float4 lightPos = { -10, 10, -10, 1 };
float4 eyePos;
float2 normuv = { 1, 1 };

void vs_main(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in		float3 tang		: TANGENT,
	in		float3 bin		: BINORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 ldirts	: TEXCOORD1,
	out		float3 vdirts	: TEXCOORD2)
{
	pos = mul(pos, matWorld);

	norm = normalize(mul(matWorldInv, float4(norm, 0)).xyz);
	tang = normalize(mul(float4(tang, 0), matWorld).xyz);
	bin = normalize(mul(float4(bin, 0), matWorld).xyz);

	ldirts = lightPos.xyz - pos.xyz;
	vdirts = eyePos.xyz - pos.xyz;

	float3x3 tbn = { tang, -bin, norm };

	ldirts = mul(tbn, ldirts);
	vdirts = mul(tbn, vdirts);

	pos = mul(pos, matViewProj);
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 ldirts	: TEXCOORD1,
	in	float3 vdirts	: TEXCOORD2,
	out	float4 color	: COLOR0)
{
	// to match the texture somewhat
	float2 t = tex * normuv;

	float3 l = normalize(ldirts);
	float3 v = normalize(vdirts);
	float3 h = normalize(l + v);
	float3 n = tex2D(mytex1, t) * 2 - 1;

	float diffuse = saturate(dot(n, l));
	float specular = saturate(dot(n, h));

	diffuse = diffuse * 0.8f + 0.2f;
	specular = pow(specular, 80);

	color = tex2D(mytex0, tex) * diffuse + specular;
	color.a = 1;
}

technique normalmap
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_main();
		pixelshader = compile ps_2_0 ps_main();
	}
}
