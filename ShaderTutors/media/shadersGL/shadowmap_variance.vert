#version 430

in vec3 my_Position;

uniform mat4 matWorld;
uniform mat4 matViewProj;

out vec2 zw;

void main()
{
	vec4 cpos = matViewProj * (matWorld * vec4(my_Position, 1));

	zw = cpos.zw;
	gl_Position = cpos;
}
