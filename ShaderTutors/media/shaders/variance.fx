
#define VARIANCE_SHADOW 1

sampler mytex0 : register(s0);
sampler mytex1 : register(s1);

matrix matWorld;
matrix matViewProj;
matrix lightVP;

float4 lightPos;

float4x4 matScale =
{
    0.5f,    0,  0, 0,
       0, -0.5f, 0, 0,
       0,    0,  1, 0,
    0.5f, 0.5f,  0, 1
};

void vs_main(
   in out float4 pos   : POSITION,
   in out float2 tex   : TEXCOORD0,
      out float4 ltov  : TEXCOORD1,
      out float d      : TEXCOORD2)
{
    pos = mul(pos, matWorld);
    
    ltov = mul(mul(pos, lightVP), matScale);
    d = length(lightPos.xyz - pos);
    
    pos = mul(pos, matViewProj);
    tex *= float2(2, 2);
}

void ps_main(
    in float2 tex   : TEXCOORD0,
    in float4 ltov  : TEXCOORD1,
    in float4 d     : TEXCOORD2,
   out float4 color : COLOR0)
{
#if VARIANCE_SHADOW
    float s;
   
    float2 ptex = ltov.xy / ltov.w;
    float2 sd = tex2D(mytex1, ptex).rg;
    
    float mean = sd.x;
    float variance = max(sd.y - sd.x * sd.x, 0.0002f);
    
    float md = mean - d;
    float pmax = variance / (variance + md * md);
    
    s = max(d <= mean, pmax);
    s = saturate(s + 0.3f);
      
    color = tex2D(mytex0, tex) * s;
    color.a = 1;
#else
    float2 texelsize = { 1.0f / 512.0f, 1.0f / 512.0f };

    float2 offsets[4] =
    {
       { -0.5f, -0.5f },
       { -0.5f, 0.5f },
       { 0.5f, -0.5f },
       { 0.5f, 0.5f }
    };
    
    float sd, s = 0;
    float2 ptex = ltov.xy / ltov.w;
    
    for( int i = 0; i < 4; ++i )
    {
        sd = tex2D(mytex1, ptex + offsets[i] * texelsize).r;
        s += (d <= sd ? 1 : 0);
    }
    
    s *= 0.25f;
    s = saturate(s + 0.3f);
 
    color = tex2D(mytex0, tex) * s;
    color.a = 1;
#endif
}

technique variance
{
    pass p0
    {
        vertexshader = compile vs_2_0 vs_main();
        pixelshader = compile ps_2_0 ps_main();
    }
}