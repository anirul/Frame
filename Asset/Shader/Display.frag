#version 330 core

in vec2 vert_texcoord;

out vec4 frag_color;

uniform sampler2D Display;

void main()
{
	vec3 rgb = texture(Display, vert_texcoord).rgb;
	frag_color = vec4(rgb, 1.0);
}