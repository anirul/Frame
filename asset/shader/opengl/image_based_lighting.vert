#version 450 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texture_coord;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    fragPos = vec3(model * vec4(in_position, 1.0));
    fragNormal = mat3(transpose(inverse(view * model))) * in_normal;
    fragTexCoord = in_texture_coord;
    gl_Position = projection * view * vec4(fragPos, 1.0);
}
