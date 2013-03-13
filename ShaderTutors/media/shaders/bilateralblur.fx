
sampler2D sampler0 : register(s0) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = linear;

	AddressU = clamp;
	AddressV = clamp;
};

sampler2D sampler1 : register(s1) = sampler_state
{
	MinFilter = point;
	MagFilter = point;
	MipFilter = point;

	AddressU = clamp;
	AddressV = clamp;
};

float2 texelSize = { 1.0f / 800.0f, 1.0f / 600.0f };

void OpaqueSample(
	float2 tex,
	float2 offset,
	float refdepth,
	in out float sumweight,
	in out float4 color)
{
	float2 off = tex + texelSize * offset;

	float d = tex2D(sampler1, off).a;
	float diff = 20000 * (d - refdepth);
	float dist = saturate(0.5 - diff * diff);
	float w = dist;

	color += tex2D(sampler0, off) * w;
	sumweight += w;
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	float2 ctex = tex - 0.5f * texelSize;
	//float2 ctex = tex;

	float refdepth = tex2D(sampler1, ctex).a;

#if 1
	float sumweight = 0.0f;
	color = 0;

	const float radius = 3;

	for( float i = -radius; i <= radius; ++i )
	{
		for( float j = -radius; j <= radius; ++j )
			OpaqueSample(ctex, float2(i, j) * 2, refdepth, sumweight, color);
	}
#else
	color = tex2D(sampler0, ctex);

	float refdepth = tex2D(sampler1, ctex).a;
	float sumweight = 1.0f;

	OpaqueSample(ctex, float2(-1,  0), refdepth, sumweight, color);
	OpaqueSample(ctex, float2( 1,  0), refdepth, sumweight, color);
	OpaqueSample(ctex, float2( 0, -1), refdepth, sumweight, color);
	OpaqueSample(ctex, float2( 0,  1), refdepth, sumweight, color);

	OpaqueSample(ctex, float2( 1, -1), refdepth, sumweight, color);
	OpaqueSample(ctex, float2(-1, -1), refdepth, sumweight, color);
	OpaqueSample(ctex, float2( 1,  1), refdepth, sumweight, color);
	OpaqueSample(ctex, float2(-1,  1), refdepth, sumweight, color);
#endif

	color /= sumweight;
	color.a = 1;
}

technique blur
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_3_0 ps_main();
	}
}
