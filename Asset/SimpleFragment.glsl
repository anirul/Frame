#version 410 core

layout(location = 0) out vec4 frag_color;

in vec3 out_normal;
in vec2 out_texcoord;

uniform sampler2D texture1;

void main()
{
	vec3 light_position = vec3(0.0, 0.0, 1.0);
	float shade = clamp(dot(light_position, out_normal), 0.0, 1.0);
	frag_color = 
		vec4(shade, shade, shade, 1.0) * 
		texture(texture1, out_texcoord);
}
