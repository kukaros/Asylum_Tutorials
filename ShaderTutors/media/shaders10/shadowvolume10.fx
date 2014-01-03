
uniform matrix matWorld;
uniform matrix matViewProj;

uniform float4 lightPos = { -10, 10, -10, 1 };	// object space
uniform float4 faceColor = { 0, 0, 0, 1 };

struct GS_Input
{
	float3 pos : TEXCOORD0;
};

struct GS_Output
{
	float4 opos : SV_Position;
};

bool IsSilhouetteEdge(
	in float center,
	in float3 v0,
	in float3 v1,
	in float3 v2)
{
	// (v0, v2) is the edge
	float3 a = v1 - v0;
	float3 b = v2 - v0;
	float4 planeeq;
	float dist;

	planeeq.xyz = cross(a, b);
	planeeq.w = -dot(planeeq.xyz, v0);

	dist = dot(planeeq, lightPos);

	// no need to extrude them twice
	if( center < 0 )
		return false;

	return (dist < 0);
}

void ExtrudeIfSilhouette(
	in float dist,
	in float3 v1,
	in float3 v2,
	in float3 v3,
	in out TriangleStream<GS_Output> stream)
{
	GS_Output	outvert;
	float4		extruded1;
	float4		extruded2;

	if( IsSilhouetteEdge(dist, v1, v2, v3) )
	{
		extruded1 = float4(v1 - lightPos.xyz, 0);
		extruded2 = float4(v3 - lightPos.xyz, 0);

		extruded1 = mul(mul(extruded1, matWorld), matViewProj);
		extruded2 = mul(mul(extruded2, matWorld), matViewProj);

		outvert.opos = mul(mul(float4(v3, 1), matWorld), matViewProj);
		stream.Append(outvert);

		outvert.opos = mul(mul(float4(v1, 1), matWorld), matViewProj);
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
	out		float3 opos		: TEXCOORD0)
{
	// world space won't work!!!
	opos = pos;
}

[maxvertexcount(12)]
void gs_extrude(
	in triangleadj GS_Input verts[6], 
	in out TriangleStream<GS_Output> stream)
{
	float4		planeeq;
	float3		a = verts[2].pos - verts[0].pos;
	float3		b = verts[4].pos - verts[0].pos;
	float		dist;

	// precalculate center triangle's plane equation
	planeeq.xyz	= cross(a, b);
	planeeq.w	= -dot(planeeq.xyz, verts[0].pos);
	dist		= dot(planeeq, lightPos);

	// extrude volume sides
	ExtrudeIfSilhouette(dist, verts[0].pos, verts[1].pos, verts[2].pos, stream);
	ExtrudeIfSilhouette(dist, verts[2].pos, verts[3].pos, verts[4].pos, stream);
	ExtrudeIfSilhouette(dist, verts[4].pos, verts[5].pos, verts[0].pos, stream);

	// front cap

	// back cap
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
