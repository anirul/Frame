#version 330 core

in vec3 vert_normal;
in vec2 vert_texcoord;
in vec3 vert_position;

layout(location = 0) out vec4 frag_color;

uniform sampler2D Color;

const vec3 light_position = vec3(-1.0, 1.0, 1.0);

void main()
{
	float shade = clamp(dot(light_position, vert_normal), 0, 1);
	vec3 color = vec3(texture(Color, vert_texcoord));
	frag_color = vec4(shade * color, 1.0);
	// frag_color = vec4(vert_position, 1.0);
	// frag_color = vec4(vert_normal, 1.0);
}
