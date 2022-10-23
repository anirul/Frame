#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

out vec3 vert_position;
out vec3 vert_color;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	mat4 pvm = projection * view * model;
	vert_position = (pvm * vec4(in_position, 1.0)).xyz;
	vert_color = in_color;
	gl_PointSize = 3.0;
	gl_Position = pvm * vec4(in_position, 1.0);
}
