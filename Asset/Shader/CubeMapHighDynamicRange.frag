#version 330 core

in vec3 vert_world_position;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;

uniform samplerCube Skybox;

void main()
{
    vec3 env_color = textureLod(Skybox, vert_world_position, 0.0).rgb;
    frag_normal = vec4(vert_world_position, 1.0);
    // deactivate the HDR as this is computed in the Bloom filter
#if 0
    env_color = env_color / (env_color + vec3(1.0));
    env_color = pow(env_color, vec3(1.0 / 2.2));
#endif
    frag_color = vec4(env_color, 1.0);
}
