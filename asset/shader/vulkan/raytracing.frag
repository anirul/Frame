#version 450

layout(location = 0) in vec2 out_uv;
layout(location = 0) out vec4 frag_color;

layout(set = 0, binding = 1) uniform sampler2D raytrace_output;

void main()
{
    frag_color = texture(raytrace_output, out_uv);
}
