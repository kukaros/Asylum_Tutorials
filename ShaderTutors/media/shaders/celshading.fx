
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

matrix matWorld;
matrix matWorldInv;
matrix matView;
matrix matProj;

float4 lightPos = { -5, 10, -10, 1 };
float2 clipPlanes = { 0.1f, 3.0f };
float2 texelSize;

static const float2 offsets[8] =
{
	{ -1, -1 },
	{  0, -1 },
	{  1, -1 },
	{ -1,  1 },

	{  0,  1 },
	{  1,  1 },
	{ -1,  0 },
	{  1,  0 }
};

void vs_main(
	in out	float4 pos		: POSITION,
	in		float3 norm		: NORMAL,
	in out	float2 tex		: TEXCOORD0,
	out		float4 normd	: TEXCOORD1,
	out		float3 ldir		: TEXCOORD2)
{
	pos = mul(pos, matWorld);
	normd.xyz = mul(matWorldInv, norm);

	ldir = lightPos.xyz - pos.xyz;
	pos = mul(pos, matView);

	normd.w = (pos.z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);
	pos = mul(pos, matProj);
}

void ps_main(
	in	float2 tex		: TEXCOORD0,
	in	float4 normd	: TEXCOORD1,
	in	float3 ldir		: TEXCOORD2,
	out	float4 color0	: COLOR0,
	out	float4 color1	: COLOR1)
{
	float3 l = normalize(ldir);
	float3 n = normalize(normd.xyz);

	float diffuse = saturate(dot(n, l));
	diffuse = tex1D(mytex1, diffuse);

	// cel shaded color
	color0 = tex2D(mytex0, tex) * diffuse;
	color0.a = 1;

	// normal + depth
	color1 = float4(n * 0.5f + 0.5f, normd.w);
}

void ps_edgedetect(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	float4 n1 = tex2D(mytex0, tex + offsets[0] * texelSize);
	float4 n2 = tex2D(mytex0, tex + offsets[1] * texelSize);
	float4 n3 = tex2D(mytex0, tex + offsets[2] * texelSize);
	float4 n4 = tex2D(mytex0, tex + offsets[3] * texelSize);
	float4 n5 = tex2D(mytex0, tex + offsets[4] * texelSize);
	float4 n6 = tex2D(mytex0, tex + offsets[5] * texelSize);
	float4 n7 = tex2D(mytex0, tex + offsets[6] * texelSize);
	float4 n8 = tex2D(mytex0, tex + offsets[7] * texelSize);

	n1.rgb = n1.rgb * 2 - 1;
	n2.rgb = n2.rgb * 2 - 1;
	n3.rgb = n3.rgb * 2 - 1;
	n4.rgb = n4.rgb * 2 - 1;
	n5.rgb = n5.rgb * 2 - 1;
	n6.rgb = n6.rgb * 2 - 1;
	n7.rgb = n7.rgb * 2 - 1;
	n8.rgb = n8.rgb * 2 - 1;

	// Sobel operator
	float3 ngx = -n1.rgb - 2 * n2.rgb - n3.rgb + n4.rgb + 2 * n5.rgb + n6.rgb;
	float3 ngy = -n1.rgb - 2 * n7.rgb - n4.rgb + n3.rgb + 2 * n8.rgb + n6.rgb;
	float3 ng = sqrt(ngx * ngx + ngy * ngy);

	ngx.x = -n1.a - 2 * n2.a - n3.a + n4.a + 2 * n5.a + n6.a;
	ngx.y = -n1.a - 2 * n7.a - n4.a + n3.a + 2 * n8.a + n6.a;

	float dg = sqrt(ngx.x * ngx.x + ngx.y * ngx.y);

	ngx.x = saturate(dot(ng, 1) - 3.5f);
	ngx.y = saturate(ngx.x + (dg * 10 - 0.4f));

	color = ngx.y;
}

void ps_final(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR0)
{
	float d = tex2D(mytex2, tex);
	float e = (1 - d);

	color = tex2D(mytex0, tex) * e;
}

technique celshading
{
	pass p0
	{
		vertexshader = compile vs_2_0 vs_main();
		pixelshader = compile ps_2_0 ps_main();
	}
}

technique edgedetect
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_2_0 ps_edgedetect();
	}
}

technique final
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_2_0 ps_final();
	}
}
