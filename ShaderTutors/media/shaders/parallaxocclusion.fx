
sampler mytex0 : register(s0) = sampler_state
{
    minfilter = linear;
    magfilter = linear;
};

sampler mytex1 : register(s1) = sampler_state
{
    minfilter = linear;
    magfilter = linear;
};

matrix matWorld;
matrix matWorldInv;
matrix matViewProj;

float4 lightPos = { -10, 10, -10, 1 };
float4 eyePos;
float heightScale = 0.2f;

/**
 * \brief Parallax offset computation
 */
float2 computeParallaxOffset(float2 dp, float2 ds, int numsteps)
{
    int i = 0;
    
    float stepsize  = 1.0 / (float)numsteps;
    float curr      = 0.0;            // mostani magasság
    float prev      = 1.0;            // elözö
    float bound     = 1.0;            // keret

    float2 offstep  = stepsize * ds;  // ennyit lépünk egy iterácioban
    float2 offset   = dp;             // itt vagyunk éppen
    float2 pt1      = 0;
    float2 pt2      = 0;

    // "bedobozoljuk" a pontot
    while( i < numsteps )
    {
        // lépünk a textúrában
        offset += offstep;
        curr = tex2D(mytex1, offset).a;
        
        // és y-on is
        bound -= stepsize;

        // ha átlépték egymást
        if( curr > bound ) 
        {   
            pt1 = float2(bound, curr);
            pt2 = float2(bound + stepsize, prev);

            i = numsteps + 1;
        }
        else
        {
            i++;
            prev = curr;
        }
    }   

    // gözöm nincs, hogy ez mégis mire gondol
    float d2 = pt2.x - pt2.y;
    float d1 = pt1.x - pt1.y;
      
    float d = d2 - d1;
    float amount = 0;
    
    if( d != 0 )
        amount = (pt1.x * d2 - pt2.x * d1) / d;
      
    return dp + ds * (1.0f - amount);
}

/**
 * \brief Vertex shader
 */
void vs_main(
   in out float4 pos    : POSITION,
       in float3 norm   : NORMAL,
       in float3 tang   : TANGENT,
       in float3 bin    : BINORMAL,
   in out float2 tex    : TEXCOORD0,
      out float3 vdir   : TEXCOORD1,
      out float3 wtan   : TEXCOORD2,
      out float3 wbin   : TEXCOORD3,
      out float3 wnorm  : TEXCOORD4)
{
    wnorm = normalize(mul(matWorldInv, float4(norm, 0)).xyz);
    wtan = normalize(mul(float4(tang, 0), matWorld).xyz);
    wbin = normalize(mul(float4(bin, 0), matWorld).xyz);
    
    pos = mul(pos, matWorld);
    vdir = pos.xyz - eyePos.xyz;
    
    pos = mul(pos, matViewProj);
}

/**
 * \brief Pixel shader
 */
void ps_main(
    in float2 tex    : TEXCOORD0,
    in float3 vdir   : TEXCOORD1,
    in float3 wtan   : TEXCOORD2,
    in float3 wbin   : TEXCOORD3,
    in float3 wnorm  : TEXCOORD4,
   out float4 color  : COLOR0)
{
    float3 p = vdir;
    float3 v = normalize(vdir);

    float3x3 tbn = { wtan, -wbin, wnorm };
    float3 s = mul(tbn, v);
    
    int numsteps = 15;
    
    // ebbe az irányba kell keresni
    float2 pd = normalize(float2(-s.x, s.y));
    
    // maximális offset kiszámolása (tg alfa segitségével)
    float sl = length(s);
    float pl = sqrt(sl * sl - s.z * s.z) / s.z; 
    
    // megkeressük az uj offsetet
    float2 dp = tex;
    float2 ds = pd * pl * heightScale;
    
    tex = computeParallaxOffset(dp, ds, numsteps);
        
    // fényvektor + átrakás tangent spacebe
    float3 l = normalize(lightPos.xyz - p);
    l = normalize(mul(tbn, l));
    
    float3 n = tex2D(mytex1, tex) * 2 - 1;
    float3 h = normalize(l - s);
    
    // világitás
    float diffuse = saturate(dot(n, l));
    float specular = saturate(dot(n, h));
    
    diffuse = diffuse * 0.8f + 0.2f;
    specular = pow(specular, 80);

    // végsö szin
    color = tex2D(mytex0, tex) * diffuse + specular;
    color.a = 1;
}

/**
 * \brief Parallax occlusion mapping
 */
technique parallaxocclusion
{
    pass p0
    {
        vertexshader = compile vs_3_0 vs_main();
        pixelshader = compile ps_3_0 ps_main();
    }
}
