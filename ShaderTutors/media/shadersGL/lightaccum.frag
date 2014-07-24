#version 430

struct LightParticle
{
	vec4 Color;
	vec4 Previous;
	vec4 Current;
	vec4 VelocityAndRadius;
};

struct ListHead
{
	uint Start;
	uint Count;
};

struct ListNode
{
	uint LightIndex;
	uint Next;
};

layout(packed, binding = 0) readonly buffer HeadBuffer {
	ListHead data[];
} headbuffer;

layout(packed, binding = 1) readonly buffer NodeBuffer {
	ListNode data[];
} nodebuffer;

layout(std140, binding = 2) readonly buffer LightBuffer {
	LightParticle data[];
} lightbuffer;

in vec4 wpos;
in vec3 wnorm;
in vec3 vdir;
in vec2 tex;

out vec4 my_FragColor0;

float Attenuate(vec3 ldir, float radius)
{
	float atten = dot(ldir, ldir) * radius;

	atten = 1.0 / (atten * 15.0 + 1.0);
	atten = (atten - 0.0625) * 1.066666;

	return clamp(atten, 0.0, 1.0);
}

void main()
{
	const int numtilesx = 50;	//
	const int numtilesy = 38;	//

	ivec2	loc = ivec2(gl_FragCoord.xy);
	ivec2	tileID = loc / ivec2(16, 16);
	int		index = tileID.y * numtilesx + tileID.x;

	uint start = headbuffer.data[index].Start;
	uint count = headbuffer.data[index].Count;
	uint nodeID = start;
	uint lightID;

	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
	vec4 lightpos;
	vec3 ldir;
	vec3 n = normalize(wnorm);
	float diff;
	float atten;
	float radius;

	for( uint i = 0; i < count; ++i )
	{
		lightID = nodebuffer.data[nodeID].LightIndex;
		nodeID = nodebuffer.data[nodeID].Next;

		lightpos = lightbuffer.data[lightID].Current;
		radius = lightbuffer.data[lightID].VelocityAndRadius.w;

		ldir = lightpos.xyz - wpos.xyz;
		atten = Attenuate(ldir, radius);

		ldir = normalize(ldir);
		diff = max(dot(ldir, n), 0) * atten;
		
		color.rgb += vec3(diff, diff, diff);
	}

	my_FragColor0 = color;

	/*
	my_FragColor0 = vec4(0.0, 1.0, 0.0, 1.0);

	if( start > 0 && count > 0 )
		my_FragColor0 = vec4(1.0, 1.0, 0.0, 1.0);

	if( start > 0 && count > 50 )
		my_FragColor0 = vec4(1.0, 0.0, 0.0, 1.0);

	my_FragColor0.a = 1.0;
	*/
}
