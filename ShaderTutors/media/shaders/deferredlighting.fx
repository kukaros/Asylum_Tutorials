
sampler mytex0 : register(s0); // albedo
sampler mytex1 : register(s1); // diffuse
sampler mytex2 : register(s2); // specular

matrix matWorld;
matrix matViewProj;

float4 matDiffuse = { 1, 1, 1, 1 };
float4 params = { 1, 1, 1, 1 }; // uv multiplier, binormal sign, has texture

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

void ps_deferredlight(
	in	float2 tex		: TEXCOORD0,
	in	float4 cpos		: TEXCOORD1,
	out	float4 color	: COLOR0)
{
	float2 ptex = cpos.xy / cpos.w;
	ptex = ptex * float2(0.5f, -0.5f) + 0.5f;

	float4 base = tex2D(mytex0, tex);
	float4 dlight = tex2D(mytex1, ptex);
	float4 slight = tex2D(mytex2, ptex);

	base.rgb = pow(base.rgb, 2.2f);
	base = lerp(matDiffuse, base, params.w);

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
