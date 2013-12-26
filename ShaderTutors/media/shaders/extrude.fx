
matrix matWorld;
matrix matViewProj;

void vs_extrude(
	in out float4 pos : POSITION)
{
	pos = mul(mul(pos, matWorld), matViewProj);
}

void ps_extrude(
	out float4 color : COLOR0)
{
	color = float4(0, 0, 0, 1);
}

technique extrude
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_extrude();
		pixelshader = compile ps_2_0 ps_extrude();
	}
}
