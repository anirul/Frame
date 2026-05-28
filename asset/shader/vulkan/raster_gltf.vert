#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec2 frag_uv;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec3 frag_skybox_dir;
layout(location = 3) out float frag_skybox;
layout(location = 4) out vec3 frag_world_pos;

layout(push_constant) uniform PushConstants
{
    mat4 projection;
    mat4 view;
    mat4 model;
    float time_s;
} pc;

void main()
{
    frag_uv = in_texcoord;
    vec4 world_pos = pc.model * vec4(in_position, 1.0);
    frag_world_pos = world_pos.xyz;
    frag_normal = normalize(transpose(inverse(mat3(pc.model))) * in_normal);
    frag_skybox_dir = in_position;
    frag_skybox = pc.time_s < 0.0 ? 1.0 : 0.0;
    if (frag_skybox > 0.5)
    {
        mat4 rotation_view = mat4(mat3(pc.view));
        mat4 rotation_model = mat4(mat3(pc.model));
        mat4 pvm = pc.projection * rotation_view * rotation_model;
        vec4 clip_pos = pvm * vec4(in_position.x, -in_position.yz, 1.0);
        gl_Position = vec4(clip_pos.xy, clip_pos.w, clip_pos.w);
    }
    else
    {
        gl_Position = pc.projection * pc.view * pc.model *
            vec4(in_position, 1.0);
    }
}
