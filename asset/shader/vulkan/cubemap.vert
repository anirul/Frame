#version 450

layout(location = 0) in vec3 in_pos;

layout(location = 0) out vec3 v_dir;

layout(push_constant) uniform PushConstants {
    mat4 projection;
    mat4 view;
    mat4 model;
    float time_s;
} pc;

void main()
{
    vec3 vert_local_pos = in_pos;
    mat4 rotation_view = mat4(mat3(pc.view));
    mat4 rotation_model = mat4(mat3(pc.model));
    mat4 pvm = pc.projection * rotation_view * rotation_model;

    vec4 clip_pos = pvm * vec4(vert_local_pos.x, -vert_local_pos.yz, 1.0);
    v_dir = vert_local_pos;
    gl_Position = vec4(clip_pos.xy, clip_pos.w, clip_pos.w);
}
