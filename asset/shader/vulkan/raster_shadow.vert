#version 450

layout(location = 0) in vec3 in_position;

layout(push_constant) uniform PushConstants
{
    mat4 light_view_projection;
    mat4 model;
} pc;

void main()
{
    gl_Position = pc.light_view_projection * pc.model * vec4(in_position, 1.0);
}
