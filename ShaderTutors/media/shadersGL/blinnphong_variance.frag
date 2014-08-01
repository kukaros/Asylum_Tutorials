#version 430

uniform sampler2D sampler0;
uniform sampler2D sampler1;		// shadowmap

uniform vec4 lightColor;

in vec4 ltov;
in vec3 wnorm;
in vec3 ldir;
in vec3 vdir;
in vec2 tex;

out vec4 my_FragColor0;

layout(early_fragment_tests) in;
void main()
{
	vec3 n = normalize(wnorm);
	vec3 l = normalize(ldir);
	vec3 v = normalize(vdir);
	vec3 h = normalize(v + l);

	float d = clamp(dot(l, n), 0.0, 1.0);
	float s = pow(clamp(dot(h, n), 0.0, 1.0), 80.0);

	vec3 projpos = (ltov.xyz / ltov.w) * 0.5 + 0.5;
	float sd = texture(sampler1, projpos.xy);
	float shadow = 1.0; //((d <= sd) ? 1.0 : 0.0);

	vec4 base = texture(sampler0, tex);

	my_FragColor0.rgb = shadow * lightColor.rgb * (base.rgb * d + vec3(s));
	my_FragColor0.rgb = vec3(sd, sd, sd) * d; //
	my_FragColor0.a = 1.0;
}
