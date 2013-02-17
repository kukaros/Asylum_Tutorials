
matrix matWorld;
matrix lightViewProj;

float4 lightPos;

void vs_main(
   in out float4 pos  : POSITION,
      out float d     : TEXCOORD0)
{
    pos = mul(pos, matWorld);
    d = length(lightPos.xyz - pos.xyz);
    
    pos = mul(pos, lightViewProj);
}

void ps_main(
    in float d       : TEXCOORD0,
   out float4 color  : COLOR0)
{
    color = float4(d, d * d, 0, 1);
}

technique distance
{
    pass p0
    {
        vertexshader = compile vs_2_0 vs_main();
        pixelshader = compile ps_2_0 ps_main();
    }
}
