#version 430

in vec3 my_Position;

uniform mat4 matWorld;
uniform mat4 matViewProj;

void main()
{
	vec4 wpos = matWorld * vec4(my_Position, 1);
	gl_Position = matViewProj * wpos;
}
