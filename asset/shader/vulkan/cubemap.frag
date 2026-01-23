#version 450

layout(location = 0) in vec3 v_dir;
layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform samplerCube Skybox;

void main()
{
    out_color = texture(Skybox, v_dir);
}
