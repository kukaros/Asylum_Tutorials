
uniform matrix matWorld;
uniform matrix matViewProj;

uniform float4 lightPos		= { -10, 10, -10, 1 };	// object space
uniform float4 faceColor	= { 0, 0, 0, 1 };
uniform float4 offset		= { 0, 0, 0, 0 };		// light to object center

struct GS_Input
{
	float4 pos : SV_Position;
	float3 origpos : TEXCOORD0;
};

struct GS_Output
{
	float4 opos : SV_Position;
};

void ExtrudeIfSilhouette(
	in float3 v1,
	in float3 v2,
	in float3 v3,
	in out TriangleStream<GS_Output> stream)
{
	GS_Output	outvert;
	float4		extruded1;
	float4		extruded2;
	float3		a = v2 - v1;
	float3		b = v3 - v1;
	float4		planeeq;
	float		dist;

	planeeq.xyz = cross(a, b);
	planeeq.w = -dot(planeeq.xyz, v1);

	dist = dot(planeeq, lightPos);

	if( dist < 0 )
	{
		extruded1 = float4(v1 - lightPos.xyz, 0);
		extruded2 = float4(v3 - lightPos.xyz, 0);

		extruded1 = mul(mul(extruded1, matWorld), matViewProj);
		extruded2 = mul(mul(extruded2, matWorld), matViewProj);

		outvert.opos = mul(mul(float4(v3 + offset.xyz, 1), matWorld), matViewProj);
		stream.Append(outvert);

		outvert.opos = mul(mul(float4(v1 + offset.xyz, 1), matWorld), matViewProj);
		stream.Append(outvert);

		outvert.opos = extruded2;
		stream.Append(outvert);

		outvert.opos = extruded1;
		stream.Append(outvert);

		stream.RestartStrip();
	}
}

// =======================================================================
//
// Shaders
//
// =======================================================================

void vs_extrude(
	in		float3 pos		: POSITION,
	out		float4 opos		: SV_Position,
	out		float3 origpos	: TEXCOORD0)
{
	// world space won't work!!!
	origpos = pos;
	opos = mul(mul(float4(pos + offset.xyz, 1), matWorld), matViewProj);
}

[maxvertexcount(18)]
void gs_extrude(
	in triangleadj GS_Input verts[6], 
	in out TriangleStream<GS_Output> stream)
{
	GS_Output	outvert;
	float4		planeeq;
	float3		a = verts[2].origpos - verts[0].origpos;
	float3		b = verts[4].origpos - verts[0].origpos;
	float		dist;

	// calculate center triangle's plane equation
	planeeq.xyz	= cross(a, b);
	planeeq.w	= -dot(planeeq.xyz, verts[0].origpos);
	dist		= dot(planeeq, lightPos);

	if( dist > 0 )
	{
		// extrude volume sides
		ExtrudeIfSilhouette(verts[0].origpos, verts[1].origpos, verts[2].origpos, stream);
		ExtrudeIfSilhouette(verts[2].origpos, verts[3].origpos, verts[4].origpos, stream);
		ExtrudeIfSilhouette(verts[4].origpos, verts[5].origpos, verts[0].origpos, stream);

		// front cap
		outvert.opos = verts[0].pos;
		stream.Append(outvert);

		outvert.opos = verts[2].pos;
		stream.Append(outvert);

		outvert.opos = verts[4].pos;
		stream.Append(outvert);

		stream.RestartStrip();

		// back cap
		outvert.opos = float4(verts[0].origpos - lightPos.xyz, 1e-5f);
		outvert.opos = mul(mul(outvert.opos, matWorld), matViewProj);
		stream.Append(outvert);

		outvert.opos = float4(verts[4].origpos - lightPos.xyz, 1e-5f);
		outvert.opos = mul(mul(outvert.opos, matWorld), matViewProj);
		stream.Append(outvert);

		outvert.opos = float4(verts[2].origpos - lightPos.xyz, 1e-5f);
		outvert.opos = mul(mul(outvert.opos, matWorld), matViewProj);
		stream.Append(outvert);
	}
}

void ps_extrude(
	out float4 color : SV_Target)
{
	color = faceColor;
}

// =======================================================================
//
// Techniques
//
// =======================================================================

technique10 extrude
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_extrude()));
		SetGeometryShader(CompileShader(gs_4_0, gs_extrude()));
		SetPixelShader(CompileShader(ps_4_0, ps_extrude()));
	}
}
