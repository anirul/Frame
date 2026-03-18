#version 460

#extension GL_EXT_ray_tracing : require

struct RayPayload
{
    vec4 hit_data;
    ivec4 index_data;
};

layout(location = 0) rayPayloadInEXT RayPayload ray_payload;

void main()
{
    ray_payload.hit_data = vec4(0.0);
    ray_payload.index_data = ivec4(-1);
}
