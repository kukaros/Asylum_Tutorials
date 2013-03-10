
sampler mytex0 : register(s0) = sampler_state
{
	MinFilter = point;
	MagFilter = point;
	MipFilter = point;

	AddressU = clamp;
	AddressV = clamp;
};

sampler mytex1 : register(s1) = sampler_state
{
	MinFilter = point;
	MagFilter = point;
	MipFilter = point;

	AddressU = clamp;
	AddressV = clamp;
};

matrix matViewProjInv;
matrix lightViewProj;

float4 lightPos;
float2 texelSize;

float2 pcfOffsets[8] =
{
	{ -1, -1 },
	{ -1,  1 },
	{  1, -1 },
	{  1,  1 },

	{ -1,  0 },
	{  1,  0 },
	{  0,  1 },
	{  0, -1 }
};

void ps_main(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	float4 normd = tex2D(mytex1, tex);
	float3 n = normalize(normd.xyz);
	float4 wpos = float4(tex.x * 2 - 1, tex.y * -2 + 1, normd.w, 1);

	wpos = mul(wpos, matViewProjInv);
	wpos /= wpos.w;

	if( dot(n, n) != 0 )
	{
		float4 ltov = mul(wpos, lightViewProj);
		float2 ptex = ltov.xy / ltov.w;
		
		ptex.x = ptex.x * 0.5f + 0.5f;
		ptex.y = ptex.y * -0.5f + 0.5f;
		
		float d = length(lightPos.xyz - wpos.xyz) - 0.3f;
		float sd;
		float t, s = 0;
		
		for( int i = 0; i < 8; ++i )
		{
			sd = tex2D(mytex0, ptex + pcfOffsets[i] * texelSize).r;

			t = (d <= sd ? 1 : 0);
			s += (sd == 0 ? 1 : t);
		}

		s *= 0.125f;
		s = saturate(s + 0.2f);
		
		color = float4(s, s, s, 1);
	}
	else
	{
		color = float4(1, 1, 1, 1);
	}
}

technique pcf
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_3_0 ps_main();
	}
}
