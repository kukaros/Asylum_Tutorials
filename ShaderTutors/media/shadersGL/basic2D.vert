#version 430

in vec3 my_Position;
in vec2 my_Texcoord0;

out vec2 tex;

void main()
{
	tex = my_Texcoord0;
	gl_Position = vec4(my_Position, 1);
}
