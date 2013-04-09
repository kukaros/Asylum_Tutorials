
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

float time = 0;
float4 lightPos = { -10, 10, -10, 1 };
float4 eyePos;

static const float2 wavedir1 = { -0.02, 0 };
static const float2 wavedir2 = { 0, -0.013 };
static const float2 wavedir3 = { 0.007, 0.007 };

static const matrix matScale =
{
	0.5,	0,		0,		0.5,
	0,		0.5,	0,		0.5, 
	0,		0,		0.5,	0.5,
	0,		0,		0,		1
};

void vs_main(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in		float3 tang		: TANGENT,
	in		float3 bin		: BINORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 ldirts	: TEXCOORD1,
	out		float3 vdirts	: TEXCOORD2,
	out		float4 tproj	: TEXCOORD3)
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
	tproj = mul(matScale, pos);
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float3 ldirts	: TEXCOORD1,
	in	float3 vdirts	: TEXCOORD2,
	in	float4 tproj	: TEXCOORD3,
	out	float4 color	: COLOR0)
{
	float2 t = tex * 0.4f;

	float3 l = normalize(ldirts);
	float3 v = normalize(vdirts);
	float3 h = normalize(l + v);
	float3 n = 0;

	n += (tex2D(mytex1, t + time * wavedir1) * 2 - 1) * 0.3f;
	n += (tex2D(mytex1, t + time * wavedir2) * 2 - 1) * 0.3f;
	n += (tex2D(mytex1, t + time * wavedir3) * 2 - 1) * 0.4f;

	n = normalize(n);
	t = tproj.xy / tproj.w;

	float4 base = tex2D(mytex0, t + n.xy * 0.02f);
	float diffuse = saturate(dot(n, l));
	float specular = saturate(dot(n, h));

	diffuse = diffuse * 0.6f + 0.4f;
	specular = pow(specular, 120);

	color = base * diffuse + specular;
	color.a = 1;
}

technique water
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_main();
		pixelshader = compile ps_2_0 ps_main();
	}
}
