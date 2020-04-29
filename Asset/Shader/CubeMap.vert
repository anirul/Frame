#version 330 core

layout (location = 0) in vec3 in_position;

out vec3 vert_local_pos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vert_local_pos = in_position;
    mat4 pvm = projection * view * model;
    gl_Position = pvm * vec4(vert_local_pos, 1.0);
}
