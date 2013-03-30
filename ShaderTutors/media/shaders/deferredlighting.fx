
sampler mytex0 : register(s0); // albedo
sampler mytex1 : register(s1); // diffuse
sampler mytex2 : register(s2); // specular
sampler mytex3 : register(s3); // envmap

matrix matWorld;
matrix matWorldInv;
matrix matViewProj;

float4 matDiffuse = { 1, 1, 1, 1 };
float4 params = { 1, 1, 1, 1 }; // uv multiplier, binormal sign, has texture
float4 scale = { 1, 1, 1, 1 };
float4 eyePos = { 0, 0, 0, 1 };

void vs_deferredlight(
	in out	float4 pos		: POSITION,
	in out	float2 tex		: TEXCOORD0,
	out		float4 cpos		: TEXCOORD1)
{
	cpos = mul(pos, matWorld);
	cpos = mul(cpos, matViewProj);

	pos = cpos;
	tex *= params.xy;
} 

void vs_deferredlight_fresnel(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float3 vdir		: TEXCOORD1,
	out		float3 wnorm	: TEXCOORD2,
	out		float4 cpos		: TEXCOORD3)
{
	float4 wpos = mul(pos, matWorld);
	cpos = mul(wpos, matViewProj);

	wnorm = mul(matWorldInv, float4(norm, 0)).xyz;
	vdir = wpos.xyz - eyePos.xyz;

	pos = cpos;
	tex *= params.xy;
} 

void ps_deferredlight(
	in	float2 tex		: TEXCOORD0,
	in	float4 cpos		: TEXCOORD1,
	out	float4 color	: COLOR0)
{
	float2 ptex = cpos.xy / cpos.w;
	ptex = ptex * float2(0.5f, -0.5f) + 0.5f;

	float2 stex = ptex * scale.xy;

	float4 base = tex2D(mytex0, tex);
	float4 dlight = tex2D(mytex1, stex);
	float4 slight = tex2D(mytex2, stex);

	base.rgb = pow(base.rgb, 2.2f);
	base = lerp(matDiffuse, base, params.w);

	color = base * dlight + slight;
	color.a = 1;
}

void ps_deferredlight_fresnel(
	in	float2 tex		: TEXCOORD0,
	in	float3 vdir		: TEXCOORD1,
	in	float3 wnorm	: TEXCOORD2,
	in	float4 cpos		: TEXCOORD3,
	out	float4 color	: COLOR0)
{
	float2 ptex = cpos.xy / cpos.w;
	ptex = ptex * float2(0.5f, -0.5f) + 0.5f;

	float2 stex = ptex * scale.xy;

	float4 base = tex2D(mytex0, tex);
	float4 dlight = tex2D(mytex1, stex);
	float4 slight = tex2D(mytex2, stex);
	float4 refl;

	base.rgb = pow(base.rgb, 2.2f);
	base = lerp(matDiffuse, base, params.w);

	float3 n = normalize(wnorm);
	float3 v = normalize(vdir);

	v = reflect(v, n);
	refl = texCUBE(mytex3, v);

	float ndotv = dot(n, v);
	float fresnel = (1 - ndotv);

	base = lerp(base, refl, fresnel);

	color = base * dlight + slight;
	color.a = 1;
}

technique deferredlight
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_deferredlight();
		pixelshader = compile ps_3_0 ps_deferredlight();
	}
}

technique deferredlight_fresnel
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs_deferredlight_fresnel();
		pixelshader = compile ps_3_0 ps_deferredlight_fresnel();
	}
}
