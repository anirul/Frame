#version 330 core

in vec3 out_local;

layout(location = 0) out vec4 frag_color;

uniform sampler2D Equirectangular;

const vec2 inv_atan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(out_local));
    vec3 color = texture(Equirectangular, uv).rgb;
    
    frag_color = vec4(color, 1.0);
}
