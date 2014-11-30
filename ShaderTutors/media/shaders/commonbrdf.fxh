
float2 BRDF_BlinnPhong(float3 wnorm, float3 ldir, float3 vdir, float shininess)
{
	float3 n = normalize(wnorm);
	float3 l = normalize(ldir);
	float3 v = normalize(vdir);
	float3 h = normalize(l + v);
	float2 irrad;

	irrad.x = saturate(dot(n, l));
	irrad.y = pow(saturate(dot(h, n)), shininess);

	return irrad;
}
