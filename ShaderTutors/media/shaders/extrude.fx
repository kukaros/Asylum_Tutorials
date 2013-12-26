
matrix matWorld;
matrix matViewProj;

void vs_main(
	in out float4 pos : POSITION)
{
	pos = mul(mul(pos, matWorld), matViewProj);
}

void ps_main(
	out float4 color : COLOR0)
{
	color = float4(0, 0, 0, 1);
}

technique basic
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_main();
		pixelshader = compile ps_2_0 ps_main();
	}
}
