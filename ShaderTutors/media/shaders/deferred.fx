
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

matrix matViewProjInv;
matrix lightViewProj;

float4 lightPos;
float4 lightColor = { 1, 1, 1, 1 };
float4 eyePos;

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

float4 pointLight(float2 tex)
{
	float4 scene	= tex2D(mytex0, tex);
	float4 normd	= tex2D(mytex1, tex);
	float4 wpos		= float4(tex.x * 2 - 1, tex.y * -2 + 1, normd.w, 1);

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

		float3 lightd = lightColor.rgb * diffuse * atten * pointLightIntensity;
		float3 lights = lightColor.rgb * specular * atten * pointLightIntensity;

		scene.rgb = pow(scene.rgb, 2.2f);
		scene.rgb = scene.rgb * lightd + lights;

		// shadow term
		float2 sd = texCUBE(mytex2, -l).rg;
		float d = length(ldir);

		float mean = sd.x + 0.04f;
		float variance = max(sd.y - sd.x * sd.x, 0.0002f);

		float md = mean - d;
		float pmax = variance / (variance + md * md);

		float t = max(d <= mean, pmax);
		float s = ((sd.x < 0.001f) ? 1.0f : t);

		// this is also sufficient btw. (but point filter then)
		//float t = (((d - sd.x) < 0.1f) ? 1.0f : 0.0f);
		//float s = ((sd.x < 0.001f) ? 1.0f : t);

		scene.rgb *= saturate(s);
	}

	return scene;
}

float4 directionalLight(float2 tex)
{
	float4 scene = tex2D(mytex0, tex);
	float4 normd = tex2D(mytex1, tex);
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

		float3 lightd = lightColor.rgb * diffuse * dirLightIntensity;
		float3 lights = lightColor.rgb * specular * dirLightIntensity;

		scene.rgb = pow(scene.rgb, 2.2f);
		scene.rgb = scene.rgb * lightd + lights;

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

		scene.rgb *= saturate(s + 0.1f);
	}

	return scene;
}

void ps_deferred_point(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR)
{
	color = pointLight(tex);
}

void ps_deferred_directional(
	in	float2 tex		: TEXCOORD0,
	out	float4 color	: COLOR)
{
	color = directionalLight(tex);
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
