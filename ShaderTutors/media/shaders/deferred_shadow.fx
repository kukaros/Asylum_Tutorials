
sampler mytex0 : register(s0) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = linear;
};

sampler mytex1 : register(s1) = sampler_state
{
	MinFilter = point;
	MagFilter = point;
	MipFilter = point;
};

sampler mytex2 : register(s2) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = linear;
};

matrix matViewProjInv;

float4 lightPos;
float4 eyePos;

float4 pointLight(float2 tex)
{
	float4 scene	= tex2D(mytex0, tex);
	float4 normd	= tex2D(mytex1, tex);
	float4 wpos		= float4(tex.x * 2 - 1, tex.y * -2 + 1, normd.w, 1);

	wpos = mul(wpos, matViewProjInv);
	wpos /= wpos.w;

	float3 ldir = lightPos.xyz - wpos.xyz;

	float3 n = normalize(normd.xyz);
	float3 l = normalize(ldir);
	float3 v = normalize(eyePos.xyz - wpos.xyz);
	float3 h = normalize(l + v);

	float diffuse = saturate(dot(n, l));
	float specular = saturate(dot(n, h));

	specular = pow(specular, 200);

	scene.rgb = pow(scene.rgb, 2.2f);
	scene.rgb = scene.rgb * diffuse + specular;

	return scene;
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	float4 scene = pointLight(tex);
	float s = tex2D(mytex2, tex).r;

	color.rgb = scene.rgb * s;
	color.rgb = pow(color.rgb, 1.0f / 2.2f);

	color.a = scene.a;
}

technique deferred_shadow
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_3_0 ps_main();
	}
}
