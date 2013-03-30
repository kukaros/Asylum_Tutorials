
sampler mytex0 : register(s0) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = point;

	AddressU = clamp;
	AddressV = clamp;
};

sampler mytex1 : register(s1) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = point;

	AddressU = clamp;
	AddressV = clamp;
};

sampler mytex2 : register(s2) = sampler_state
{
	MinFilter = linear;
	MagFilter = linear;
	MipFilter = point;

	AddressU = clamp;
	AddressV = clamp;
};

sampler mytex3 : register(s3) = sampler_state
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
float4 lightColor = { 1, 1, 1, 1 };
float4 eyePos;
float4 scale = { 1, 1, 1, 1 };

static const float dirLightIntensity = 1;
static const float pointLightIntensity = 1.5f;

float attenuate(float3 ldir)
{
	ldir /= lightPos.w;

	float atten = dot(ldir, ldir);
	float att_s = 15;

	atten = 1.0f / (atten * att_s + 1);
	att_s = 1.0f / (att_s + 1);

	atten = atten - att_s;
	atten /= (1.0f - att_s);

	return saturate(atten);
}

float2 pointLight(float2 tex, float4 normd)
{
	float2 irrad = 0;
	float4 wpos = float4(tex.x * 2 - 1, tex.y * -2 + 1, normd.w, 1);

	if( normd.w > 0 )
	{
		wpos = mul(wpos, matViewProjInv);
		wpos /= wpos.w;

		float3 ldir = lightPos.xyz - wpos.xyz;

		float3 n = normalize(normd.xyz);
		float3 l = normalize(ldir);
		float3 v = normalize(eyePos.xyz - wpos.xyz);
		float3 h = normalize(l + v);

		float diffuse = saturate(dot(n, l));
		float specular = saturate(dot(n, h));
		float atten = attenuate(ldir);

		specular = pow(specular, 200);

		// shadow term
		float2 sd = texCUBE(mytex2, -l).rg;
		float d = length(ldir);

		float mean = sd.x;
		float variance = max(sd.y - sd.x * sd.x, 0.0002f);

		float md = mean - d;
		float pmax = variance / (variance + md * md);

		float t = max(d <= mean, pmax);
		float s = ((sd.x < 0.001f) ? 1.0f : t);

		// this is also sufficient btw. (but point filter then)
		//float t = (((d - sd.x) < 0.1f) ? 1.0f : 0.0f);
		//float s = ((sd.x < 0.001f) ? 1.0f : t);

		s = saturate(s);

		irrad.x = diffuse * atten * pointLightIntensity * s;
		irrad.y = specular * atten * pointLightIntensity * s;
	}

	return irrad;
}

float2 directionalLight(float2 tex, float4 normd)
{
	float2 irrad = 0;
	float4 wpos = float4(tex.x * 2 - 1, tex.y * -2 + 1, normd.w, 1);

	if( normd.w > 0 )
	{
		wpos = mul(wpos, matViewProjInv);
		wpos /= wpos.w;

		float3 n = normalize(normd.xyz);
		float3 l = normalize(lightPos.xyz);
		float3 v = normalize(eyePos.xyz - wpos.xyz);
		float3 h = normalize(l + v);

		float diffuse = saturate(dot(n, l));
		float specular = saturate(dot(n, h));

		specular = pow(specular, 200);

		// shadow term
		float4 lpos = mul(wpos, lightViewProj);
		float2 ltex = (lpos.xy / lpos.w) * float2(0.5f, -0.5f) + 0.5f;

		float2 sd = tex2D(mytex2, ltex).rg;
		float d = length(lightPos.w * lightPos.xyz - wpos.xyz);

		float mean = sd.x;
		float variance = max(sd.y - sd.x * sd.x, 0.0002f);

		float md = mean - d;
		float pmax = variance / (variance + md * md);

		float t = max(d <= mean, pmax);
		float s = ((sd.x < 0.001f) ? 1.0f : t);

		s = saturate(s + 0.1f);

		irrad.x = diffuse * dirLightIntensity * s;
		irrad.y = specular * dirLightIntensity * s;
	}

	return irrad;
}

void ps_deferred_point(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR)
{
	float4 scene = tex2D(mytex0, tex);
	float4 normd = tex2D(mytex1, tex);
	float2 irrad;

	normd.w = tex2D(mytex3, tex).r;
	irrad = pointLight(tex, normd);

	scene.rgb = pow(scene.rgb, 2.2f);
	color.rgb = scene.rgb * lightColor.rgb * irrad.x + lightColor.rgb * irrad.y;

	color.a = scene.a;
}

void ps_deferred_directional(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR)
{
	float4 scene = tex2D(mytex0, tex);
	float4 normd = tex2D(mytex1, tex);
	float2 irrad;

	normd.w = tex2D(mytex3, tex).r;
	irrad = directionalLight(tex, normd);

	scene.rgb = pow(scene.rgb, 2.2f);
	color.rgb = scene.rgb * lightColor.rgb * irrad.x + lightColor.rgb * irrad.y;

	color.a = scene.a;
}

void ps_irradiance_point(
	in	float2 tex		: TEXCOORD0,
	out	float4 diffuse	: COLOR0,
	out	float4 specular	: COLOR1)
{
	float2 stex = tex * scale.xy;
	float4 normd = tex2D(mytex0, stex);

	normd.w = tex2D(mytex1, stex).r;

	float2 irrad = pointLight(tex, normd);

	diffuse = float4(irrad.x * lightColor.rgb, 1);
	specular = float4(irrad.y * lightColor.rgb, 1);
}

void ps_irradiance_directional(
	in	float2 tex		: TEXCOORD0,
	out	float4 diffuse	: COLOR0,
	out	float4 specular	: COLOR1)
{
	float2 stex = tex * scale.xy;
	float4 normd = tex2D(mytex0, stex);

	normd.w = tex2D(mytex1, stex).r;

	float2 irrad = directionalLight(tex, normd);
	
	diffuse = float4(irrad.x * lightColor.rgb, 1);
	specular = float4(irrad.y * lightColor.rgb, 1);
}

technique deferred_point
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_3_0 ps_deferred_point();
	}
}

technique deferred_directional
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_3_0 ps_deferred_directional();
	}
}

technique irradiance_point
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_3_0 ps_irradiance_point();
	}
}

technique irradiance_directional
{
	pass p0
	{
		vertexshader = null;
		pixelshader = compile ps_3_0 ps_irradiance_directional();
	}
}
