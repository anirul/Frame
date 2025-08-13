#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 out_normal;
out vec3 out_world_position;
out vec2 out_uv;

void main()
{
    // Transform position to world space
    vec4 world_pos = model * vec4(in_position, 1.0);
    out_world_position = world_pos.xyz;

    // Normal: transform by inverse transpose of u_Model
    out_normal = mat3(transpose(inverse(model))) * in_normal;

    // UV
    out_uv = in_texcoord;

    // Finally, transform for the camera
    gl_Position = projection * view * world_pos;
}
