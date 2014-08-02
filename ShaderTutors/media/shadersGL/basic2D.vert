#version 430

in vec3 my_Position;
in vec2 my_Texcoord0;

uniform mat4 matTexture;

out vec2 tex;

void main()
{
	tex = (matTexture * vec4(my_Texcoord0, 0, 1)).xy;
	gl_Position = vec4(my_Position, 1);
}
