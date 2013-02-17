
sampler mytex0 : register(s0) = sampler_state
{
    MinFilter = linear;
    MagFilter = linear;
    MipFilter = linear;
};

sampler mytex1 : register(s1) = sampler_state
{
    MinFilter = point;
    MagFilter = point;
    MipFilter = point;
};

sampler mytex2 : register(s2) = sampler_state
{
    MinFilter = linear;
    MagFilter = linear;
    MipFilter = linear;
};

matrix matViewProjInv;

float4 lightPos;
float4 eyePos;

void ps_main(
    in float2 tex    : TEXCOORD0,
   out float4 color  : COLOR0)
{
    float4 scene = tex2D(mytex0, tex);
    float4 normd = tex2D(mytex1, tex);
    float3 n = normalize(normd.xyz);
    float4 wpos = float4(tex.x * 2 - 1, tex.y * -2 + 1, normd.w, 1);
    
    wpos = mul(wpos, matViewProjInv);
    wpos /= wpos.w;
    
    if( dot(n, n) != 0 )
    {
        // blinn-phong
        float3 l = normalize(lightPos.xyz - wpos.xyz);
		float3 v = normalize(eyePos.xyz - wpos.xyz);
		float3 h = normalize(l + v);
    
		float diffuse = saturate(dot(n, l));
		float specular = saturate(dot(n, h));
	    
		diffuse = diffuse * 0.8f + 0.2f;
		specular = pow(specular, 60);
		
		float s = tex2D(mytex2, tex).r;
		color = (scene * diffuse + specular) * s;
		//color = tex2D(mytex2, tex);
		color.a = 1;
	}
	else
	{
	    color = scene;
	    color.a = 0;
	}
}

technique deferred
{
    pass p0
    {
        vertexshader = null;
        pixelshader = compile ps_3_0 ps_main();
    }
}
