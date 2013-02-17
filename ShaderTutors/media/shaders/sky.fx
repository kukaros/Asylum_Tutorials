
matrix matWorld;
matrix matViewProj;
float4 eyePos;

samplerCUBE mytex0 : register(s0);

void vs_sky(
    in out float4 pos   : POSITION,
       out float3 view  : TEXCOORD0)
{
    pos = mul(pos, matWorld);
    view = pos.xyz - eyePos.xyz;
    
    pos = mul(pos, matViewProj);
}

void ps_sky(
    in float3 view   : TEXCOORD0,
   out float4 color0 : COLOR0)
{
    color0 = float4(texCUBE(mytex0, view).rgb, 1);
}

technique sky
{
    pass p0
    {
        vertexshader = compile vs_2_0 vs_sky();
        pixelshader = compile ps_2_0 ps_sky();
    }
}
