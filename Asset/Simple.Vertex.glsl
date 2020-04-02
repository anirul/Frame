#version 410 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

out vec3 out_normal;
out vec2 out_texcoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	out_normal = vec3(model * vec4(in_normal, 1.0));
	out_texcoord = in_texcoord;
	mat4 mvp = projection * view * model;
	gl_Position = mvp * vec4(in_position, 1.0);
}
