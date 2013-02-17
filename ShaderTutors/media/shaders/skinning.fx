
#define MAX_MATRICES 26

float4x3 matBones[MAX_MATRICES];
float4x4 matViewProj;

int numBones;

sampler2D mytex0 : register(s0);

void vs_main(
    in out float4 pos      : POSITION,
        in float4 weights  : BLENDWEIGHT,
        in float4 indices  : BLENDINDICES,
        in float3 norm     : NORMAL,
    in out float2 tex      : TEXCOORD0,
      uniform int bones)
{
    float3 bpos = 0.0f;
    float3 bnorm = 0.0f;

    int4 ind = D3DCOLORtoUBYTE4(indices);
    int blendindices[4] = (int[4])ind;
    
    float blendweights[4] = (float[4])weights;
    float last = 0;

    for( int i = 0; i < bones - 1; ++i )
    {
        last += blendweights[i];
        
        bpos += mul(pos, matBones[blendindices[i]]) * blendweights[i];
        bnorm += mul(norm, matBones[blendindices[i]]) * blendweights[i];
    }
    
    last = 1 - last;
    
    bpos += mul(pos, matBones[blendindices[bones - 1]]) * last;
    bnorm += mul(norm, matBones[blendindices[bones - 1]]) * last;

    pos = mul(float4(bpos.xyz, 1), matViewProj);
}

void ps_main(
    in float2 tex    : TEXCOORD0,
   out float4 color  : COLOR0)
{
    color = tex2D(mytex0, tex);
    color.a = 1;
}

vertexshader vsarray[4] =
{
    compile vs_2_0 vs_main(1),
    compile vs_2_0 vs_main(2),
    compile vs_2_0 vs_main(3),
    compile vs_2_0 vs_main(4)
}; 

technique skinning
{
    pass p0
    {
        vertexshader = vsarray[numBones];
        pixelshader = compile ps_2_0 ps_main();
    }
}
