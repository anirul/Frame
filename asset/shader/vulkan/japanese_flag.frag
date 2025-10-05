#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

const vec3 kBackgroundColor = vec3(1.0);
const vec3 kCircleColor = vec3(0.9, 0.0, 0.0);
const float kAspectRatio = 1280.0 / 720.0;
const float kCircleRadius = 0.3;

void main()
{
    vec2 centered = fragUV - vec2(0.5);
    centered.x *= kAspectRatio;
    float dist = length(centered);
    float mask = step(kCircleRadius, dist);
    vec3 color = mix(kCircleColor, kBackgroundColor, mask);
    outColor = vec4(color, 1.0);
}
