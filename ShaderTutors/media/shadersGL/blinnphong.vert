
#version 150

in vec3 my_Position;
in vec3 my_Normal;

uniform mat4 matWorld;
uniform mat4 matViewProj;

uniform vec4 lightPos;
uniform vec4 eyePos;

out vec3 wnorm;
out vec3 vdir;
out vec3 ldir;

void main()
{
	vec4 wpos = matWorld * vec4(my_Position, 1);

	ldir = lightPos.xyz - wpos.xyz;
	vdir = eyePos.xyz - wpos.xyz;

	wnorm = (matWorld * vec4(my_Normal, 0)).xyz;
	gl_Position = matViewProj * wpos;
}
