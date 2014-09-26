
sampler2D sampler0 : register(s0) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = none;
};

uniform float2 texelSize;

void ps_blur(
	in	float2 tex		: TEXCOORD0,
	out	float4 color0	: COLOR0)
{
	color0 = tex2D(sampler0, tex);;

	color0 += tex2D(sampler0, tex + texelSize * float2(-2, -2));
	color0 += tex2D(sampler0, tex + texelSize * float2(-1, -1));
	color0 += tex2D(sampler0, tex + texelSize * float2(1, 1));
	color0 += tex2D(sampler0, tex + texelSize * float2(2, 2));

	color0 *= 0.2f;
}

technique blur
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_2_0 ps_blur();
	}
}
