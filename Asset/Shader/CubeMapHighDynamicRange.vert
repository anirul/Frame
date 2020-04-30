#version 330 core

layout (location = 0) in vec3 in_position;

out vec3 vert_world_position;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vert_world_position = in_position;
    // Remove translation.
    mat4 rotation_view = mat4(mat3(view));
    mat4 rotation_model = mat4(mat3(model));
    // Compute the new PVM matrix.
    mat4 pvm = projection * rotation_view * rotation_model;

    vec4 clip_pos = pvm * vec4(vert_world_position, 1.0);
    gl_Position = clip_pos.xyww;
}
