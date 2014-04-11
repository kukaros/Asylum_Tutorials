
sampler2D sampler0 : register(s0) = sampler_state // shadow
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = none;

	AddressU = clamp;
	AddressV = clamp;
};

float2 texelSize = { 1.0f / 400.0f, 1.0f / 300.0f };
float blurPass = 0;

float4 blurColors[4] =
{
	float4(1, 1, 1, 1),
	float4(0.75f, 0.75f, 1, 1),
	float4(0.5f, 0.5f, 1, 1),
	float4(0.25f, 0.25f, 1, 1)
};

void ps_brightpass(
	in float2 tex		: TEXCOORD0,
	out float4 color0	: COLOR)
{
	float4 base = tex2D(sampler0, tex);
	float threshold = 0.8f;

	color0 = saturate(base - threshold / (1 - threshold));
	color0.a = 1;
}

void ps_blur(
	in float2 tex		: TEXCOORD0,
	out float4 color0	: COLOR0)
{
	color0 = 0;

	float b = pow(8, blurPass);
	float a = 0.93f;

	float2 off = float2(1, 0);
	float2 stex;

	for( int i = 0; i < 8; ++i )
	{
		stex = tex + b * i * texelSize * off;

		color0 +=
			tex2D(sampler0, stex) * pow(a, b * i);
	}

	color0 *= blurColors[(int)blurPass];
	color0.a = 1;
}

void ps_combine(
	in float2 tex		: TEXCOORD0,
	out float4 color0	: COLOR0)
{
	color0 = tex2D(sampler0, tex);
}

technique brightpass
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_brightpass();
	}
}

technique blur
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_blur();
	}
}

technique combine
{
	pass p0
	{
		pixelshader = compile ps_2_0 ps_combine();
	}
}
