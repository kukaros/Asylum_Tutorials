
matrix matWorld;
matrix matWorldInv;
matrix matViewProj;

float2 uv = { 1, 1 };
float4 eyePos;
float roughness = 0.2f;

sampler2D mytex0    : register(s0);
sampler2D mytex1    : register(s1);

samplerCUBE mytex2  : register(s2);
samplerCUBE mytex3  : register(s3);

void vs_fresnel(
     in out float4 pos   : POSITION,
         in float3 norm  : NORMAL,
     in out float2 tex   : TEXCOORD0,
        out float3 view  : TEXCOORD1,
        out float3 wnorm : TEXCOORD2)
{
    pos = mul(pos, matWorld);
    wnorm = mul(matWorldInv, float4(norm, 0)).xyz;
    
    tex *= uv;

    view = pos.xyz - eyePos.xyz;
    pos = mul(pos, matViewProj);
}

void ps_fresnel(
      in float2 tex    : TEXCOORD0,
      in float3 view   : TEXCOORD1,
      in float3 wnorm  : TEXCOORD2,
     out float4 color0 : COLOR0)
{
    wnorm = normalize(wnorm);
    view = normalize(view);
    
    float3 r = reflect(view, wnorm).xyz;
    
    float4 base = tex2D(mytex0, tex);
    float4 ref = texCUBE(mytex2, r);
    float4 spec = texCUBE(mytex3, r);
    
    float2 ftex = float2(dot(wnorm, -view), 0.75f);
    float fresnel = tex2D(mytex1, ftex).r;
    
    float r1w = max(ftex.x, 0);
    r1w = r1w * (roughness - 1) + 1;
    
    float4 s = lerp(ref, spec, r1w);
    float4 d = fresnel * (1 - base) + base;

    color0 = s * d;
    color0.a = 1;
}

technique fresnel
{
    pass p0
    {
        vertexshader = compile vs_2_0 vs_fresnel();
        pixelshader = compile ps_2_0 ps_fresnel();
    }
}
