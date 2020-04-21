#version 330 core

layout (location = 0) in vec3 in_position;

out vec3 out_local;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    out_local = in_position;
    gl_Position = projection * view * model * vec4(out_local, 1.0);
}
