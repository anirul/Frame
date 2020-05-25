#version 330 core

in vec2 vert_texcoord;

out vec4 frag_color;

uniform sampler2D Display;

uniform float exposure;
uniform float gamma;

void main()
{
	vec3 rgb = texture(Display, vert_texcoord).rgb;
	// Tone mapping.
    vec3 result = vec3(1.0) - exp(-rgb * exposure);
    // Also gamma correct while we're at it.
    result = pow(result, vec3(1.0 / gamma));
    frag_color = vec4(result, 1.0);
}