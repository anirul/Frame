#version 460

#extension GL_EXT_ray_tracing : require

struct RayPayload
{
    vec4 hit_data;
    ivec4 index_data;
};

layout(location = 0) rayPayloadInEXT RayPayload ray_payload;
hitAttributeEXT vec2 hit_attribs;

void main()
{
    ray_payload.hit_data = vec4(1.0, gl_HitTEXT, hit_attribs.x, hit_attribs.y);
    ray_payload.index_data = ivec4(
        gl_PrimitiveID,
        int(gl_InstanceCustomIndexEXT),
        0,
        0);
}
