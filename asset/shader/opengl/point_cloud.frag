#version 330 core

in vec3 vert_position;
in vec3 vert_color;

layout(location = 0) out vec4 frag_color;

void main()
{
	frag_color = vec4(abs(vert_position), 1.0);
}
