#version 450

layout(location = 0) out vec2 out_uv;

void main()
{
    const vec2 positions[6] = vec2[](
        vec2(-1.0, -1.0),
        vec2(1.0, -1.0),
        vec2(1.0, 1.0),
        vec2(-1.0, -1.0),
        vec2(1.0, 1.0),
        vec2(-1.0, 1.0));
    out_uv = positions[gl_VertexIndex] * 0.5 + 0.5;
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
