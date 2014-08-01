#version 430

in vec2 zw;
out vec4 my_FragColor0;

void main()
{
	float d = zw.x / zw.y; // [-1, 1]
	d = d * 0.5 + 0.5; // [0, 1]

	my_FragColor0 = vec4(d, d * d, 0.0, 0.0);
}
