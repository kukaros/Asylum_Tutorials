
sampler mytex0 : register(s0) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
};

matrix matWVP;

void vs_main(
	in out float4 pos	: POSITION,
	in out float2 tex	: TEXCOORD0)
{
	pos = mul(pos, matWVP);
} 

void ps_main(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	color = tex2D(mytex0, tex);
}

technique textured
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_main();
		pixelshader = compile ps_2_0 ps_main();
	}
}
