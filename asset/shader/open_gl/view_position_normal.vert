#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

out vec3 vert_position;
out vec2 vert_texcoord;
out vec3 vert_normal;

uniform bool inverted_normals;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vec4 view_pos = view * model * vec4(in_position, 1.0);
    vert_position = view_pos.xyz; 
    vert_texcoord = in_texcoord;
    
    mat3 normal_matrix = transpose(inverse(mat3(view * model)));
    vert_normal = normal_matrix * ((inverted_normals) ? -in_normal : in_normal);
    
    gl_Position = projection * view_pos;
}