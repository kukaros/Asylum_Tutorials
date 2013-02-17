
matrix matWVP;

void vs_main(
   in out float4 pos : POSITION)
{
    // vigyázat, hogy merröl szorzunk!
    pos = mul(pos, matWVP);
} 

void ps_main(
   out float4 color : COLOR0)
{
    // zöld
    color = float4(0, 1, 0, 1);
}

technique basic
{
    pass p0
    {
        vertexshader = compile vs_2_0 vs_main();
        pixelshader = compile ps_2_0 ps_main();
    }
}
