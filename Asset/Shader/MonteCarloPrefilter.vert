#version 330 core
layout (location = 0) in vec3 in_position;

out vec3 vert_world_position;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    vert_world_position = in_position;  
    gl_Position =  projection * view * vec4(vert_world_position, 1.0);
}