#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

out vec3 out_normal;
out vec3 out_world;
out vec2 out_texcoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	out_normal = mat3(model) * in_normal;
	out_world = vec3(model * vec4(in_position, 1.0));
	out_texcoord = vec2(in_texcoord.x, 1.0 - in_texcoord.y);

	gl_Position = projection * view * vec4(out_world, 1.0);
}
