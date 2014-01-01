
matrix matWorld;
matrix matViewProj;

float4 ambient = { 0, 0, 0, 1 };	// tell me a better name...

void vs_extrude(
	in out float4 pos : POSITION)
{
	pos = mul(mul(pos, matWorld), matViewProj);
}

void vs_zpass(
	in float3 pos : POSITION,
	out float4 opos : POSITION)
{
	opos = mul(mul(float4(pos, 1), matWorld), matViewProj);
}

void ps_extrude(
	out float4 color : COLOR0)
{
	color = ambient;
}

technique extrude
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_extrude();
		pixelshader = compile ps_2_0 ps_extrude();
	}
}

technique zpass
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_zpass();
		pixelshader = compile ps_2_0 ps_extrude();
	}
}
