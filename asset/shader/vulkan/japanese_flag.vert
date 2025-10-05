#version 450

layout(location = 0) out vec2 fragUV;

const vec2 kPositions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

const vec2 kUVs[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

void main()
{
    gl_Position = vec4(kPositions[gl_VertexIndex], 0.0, 1.0);
    fragUV = kUVs[gl_VertexIndex];
}
