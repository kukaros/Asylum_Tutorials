#version 430

#define MAX_LAYERS	4

// DONT'T TRUST AMD!!!

struct ListHead
{
	uvec4 StartAndCount;
};

struct ListNode
{
	uvec4 ColorDepthNext;
};

layout(std140, binding = 0) buffer HeadBuffer {
	ListHead data[];
} headbuffer;

layout(std140, binding = 1) buffer NodeBuffer {
	ListNode data[];
} nodebuffer;

layout(binding = 0) uniform atomic_uint nextInsertionPoint;

uniform sampler2D sampler0;
uniform vec4 matAmbient;
uniform int screenWidth;

in vec2 tex;

void main()
{
	vec4	base = texture(sampler0, tex);
	vec4	color = base * matAmbient;

	ivec2	fragID = ivec2(gl_FragCoord.xy);
	int		index = fragID.y * screenWidth + fragID.x;
	uint	depth = floatBitsToUint(gl_FragCoord.z);
	uint	test;

	uint count = headbuffer.data[index].StartAndCount.y;
	uint node;
	uint prev = 0xFFFFFFFF;
	uint next = headbuffer.data[index].StartAndCount.x;

	// find insertion point
	for( uint i = 0; i < count; ++i )
	{
		test = nodebuffer.data[next].ColorDepthNext.y;

		if( test < depth )
			break;

		if( prev != 0xFFFFFFFF && count >= MAX_LAYERS )
			nodebuffer.data[prev].ColorDepthNext.xy = nodebuffer.data[next].ColorDepthNext.xy;

		prev = next;
		next = nodebuffer.data[next].ColorDepthNext.z;
	}

	if( prev != 0xFFFFFFFF && count >= MAX_LAYERS )
	{
		nodebuffer.data[prev].ColorDepthNext.x = packUnorm4x8(color);
		nodebuffer.data[prev].ColorDepthNext.y = depth;
	}
	else
	{
		node = atomicCounterIncrement(nextInsertionPoint);

		if( prev == 0xFFFFFFFF )
		{
			// insert as head
			nodebuffer.data[node].ColorDepthNext.x = packUnorm4x8(color);
			nodebuffer.data[node].ColorDepthNext.y = depth;
			nodebuffer.data[node].ColorDepthNext.z = next;
			nodebuffer.data[node].ColorDepthNext.w = 0;

			headbuffer.data[index].StartAndCount.x = node;
			headbuffer.data[index].StartAndCount.y = min(count + 1, MAX_LAYERS);
		}
		else
		{
			// insert normally
			nodebuffer.data[prev].ColorDepthNext.z = node;

			nodebuffer.data[node].ColorDepthNext.x = packUnorm4x8(color);
			nodebuffer.data[node].ColorDepthNext.y = depth;
			nodebuffer.data[node].ColorDepthNext.z = next;
			nodebuffer.data[node].ColorDepthNext.w = 0;

			headbuffer.data[index].StartAndCount.y = count + 1;
		}
	}
}
