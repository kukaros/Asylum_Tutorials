#version 430

struct ListHead
{
	int Start;
	int Count;
};

struct ListNode
{
	int LightIndex;
	int Next;
};

layout(std140, binding = 0) readonly buffer HeadBuffer {
	ListHead data[];
} headbuffer;

layout(std140, binding = 1) readonly buffer NodeBuffer {
	ListNode data[];
} nodebuffer;

out vec4 color;

void main()
{
	const int numtilesx = 50;	//

	ivec2	loc = ivec2(gl_FragCoord.xy);
	ivec2	tileID = loc / ivec2(16, 16);
	int		index = tileID.y * numtilesx + tileID.x;

	color = vec4(0.0, 1.0, 0.0, 1.0);

	int start = headbuffer.data[index].Start;

	if( start < 25 )
		color = vec4(1.0, 0.0, 0.0, 1.0);
}
