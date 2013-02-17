
sampler2D sampler0 : register(s0);

float2 texelsize;

static const float2 offsetsx[5] =
{
    { -2, 0 },
    { -1, 0 },
    { 0,  0 },
    { 1, 0 },
    { 2,  0 }
};

static const float2 offsetsy[5] =
{
    { 0, -2 },
    { 0, -1 },
    { 0,  0 },
    { 0,  1 },
    { 0,  2 }
};

void ps_blurx(
    in float2 tex    : TEXCOORD0,
   out float4 color0 : COLOR0)
{
    color0 = 0;

    for( int i = 0; i < 5; ++i )
        color0 += tex2D(sampler0, tex + texelsize * offsetsx[i]);
    
    color0 /= 5;
    color0.a = 1;
}

void ps_blury(
    in float2 tex    : TEXCOORD0,
   out float4 color0 : COLOR0)
{
    color0 = 0;

    for( int i = 0; i < 5; ++i )
        color0 += tex2D(sampler0, tex + texelsize * offsetsy[i]);
    
    color0 /= 5;
    color0.a = 1;
}

technique blurx
{
    pass p0
    {
        pixelshader = compile ps_2_0 ps_blurx();
    }
}

technique blury
{
    pass p0
    {
        pixelshader = compile ps_2_0 ps_blury();
    }
}

