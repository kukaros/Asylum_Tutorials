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

	float diff = clamp(dot(l, n), 0.0, 1.0);
	float spec = pow(clamp(dot(h, n), 0.0, 1.0), 80.0);

	vec4 base = texture(sampler0, tex);
	vec3 projpos = (ltov.xyz / ltov.w) * 0.5 + 0.5;
	vec2 sd = texture(sampler1, projpos.xy).xy;
	
	float mean = sd.x;
	float variance = max(sd.y - sd.x * sd.x, 0.0002);
	float md = mean - projpos.z;
	float pmax = variance / (variance + md * md);
	float t = ((projpos.z <= mean) ? 1.0 : 0.0);
	float s;

	t = max(t, pmax);
	s = ((sd.x < 0.001) ? 1.0 : t);
	s = clamp(s, 0.0, 1.0);

	my_FragColor0.rgb = s * lightColor.rgb * (base.rgb * diff + vec3(spec));
	my_FragColor0.a = 1.0;
}
