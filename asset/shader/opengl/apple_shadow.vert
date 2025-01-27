#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 light_projection_view;

out vec3 out_normal;
out vec3 out_world_pos;
out vec2 out_uv;
out vec4 out_light_space_pos;

void main()
{
 // Transform position to world space
    vec4 world_pos = model * vec4(in_position, 1.0);
    out_world_pos = world_pos.xyz;

    // Normal: transform by inverse transpose of u_Model
    out_normal = mat3(transpose(inverse(model))) * in_normal;

    // UV
    out_uv = in_texcoord;

    // Compute light-space position for shadow lookup
    out_light_space_pos = light_projection_view * world_pos;

    // Finally, transform for the camera
    gl_Position = projection * view * world_pos;
}
