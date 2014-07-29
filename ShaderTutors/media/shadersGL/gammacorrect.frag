#version 430

uniform sampler2D sampler0;

in vec2 tex;
out vec4 my_FragColor0;

void main()
{
	vec4 base = texture(sampler0, tex);

	base.rgb = pow(base.rgb, vec3(1.0 / 2.2));
	my_FragColor0 = base;
}
