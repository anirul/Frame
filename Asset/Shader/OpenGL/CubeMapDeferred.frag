#version 330 core

in vec3 vert_world_position;

layout (location = 0) out vec4 frag_ambient;
layout (location = 1) out vec4 frag_normal;
layout (location = 2) out vec4 frag_mra;
layout (location = 3) out vec4 frag_position;

uniform samplerCube Skybox;

void main()
{
    vec3 env_color = textureLod(Skybox, vert_world_position, 0.0).rgb;
    frag_ambient = vec4(env_color, 1.0);
    frag_normal = vec4(normalize(vert_world_position), 1.0);
    frag_mra = vec4(0.0, 0.0, 0.0, 1.0);
    frag_position = vec4(0.0, 0.0, 0.0, 1.0);
}
