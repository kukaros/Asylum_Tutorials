
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

matrix matViewProjInv;

float4 lightPos;
float4 lightColor = { 1, 1, 1, 1 };
float4 eyePos;

const float dirLightIntensity = 1; //0.2f;
const float pointLightIntensity = 2;

float Attenuate(float3 ldir)
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

	wpos = mul(wpos, matViewProjInv);
	wpos /= wpos.w;

	float3 ldir = lightPos.xyz - wpos.xyz;

	float3 n = normalize(normd.xyz);
	float3 l = normalize(ldir);
	float3 v = normalize(eyePos.xyz - wpos.xyz);
	float3 h = normalize(l + v);

	float diffuse = saturate(dot(n, l));
	float specular = saturate(dot(n, h));
	float atten = Attenuate(ldir);

	specular = pow(specular, 200);

	float3 lightd = lightColor.rgb * diffuse * atten * pointLightIntensity;
	float3 lights = lightColor.rgb * specular * atten * pointLightIntensity;

	scene.rgb = pow(scene.rgb, 2.2f);
	scene.rgb = scene.rgb * lightd + lights;

	return scene;
}

float4 directionalLight(float2 tex)
{
	float4 scene = tex2D(mytex0, tex);
	float4 normd = tex2D(mytex1, tex);
	float4 wpos = float4(tex.x * 2 - 1, tex.y * -2 + 1, normd.w, 1);

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
