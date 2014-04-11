
sampler mytex0 : register(s0) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = point;

	AddressU = clamp;
	AddressV = clamp;
};

void ps_main(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	float4 base = tex2D(mytex0, tex);

	color.rgb = pow(base.rgb, 1.0f / 2.2f);
	color.a = 1;
}

technique basic
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_2_0 ps_main();
	}
}
