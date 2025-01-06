#version 410 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texture_coord;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 env_map_model = mat4(1.0);

void main()
{
    // Transform the vertex position to clip space.
    fragPos = vec3(model * vec4(in_position, 1.0));
    // Transform the normal to world space.
    vec3 worldNormal = transpose(inverse(mat3(model))) * in_normal;
    // Now transform worldNormal into environment space:
    fragNormal = mat3(inverse(env_map_model)) * worldNormal;
    // Transform the texture coordinates.
    fragTexCoord = in_texture_coord;
    gl_Position = projection * view * vec4(fragPos, 1.0);
}
