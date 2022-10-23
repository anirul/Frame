#version 330 core

layout (location = 0) in vec3 in_position;

out vec3 vert_local;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vert_local = in_position;
    gl_Position = projection * view * vec4(vert_local, 1.0);
}
