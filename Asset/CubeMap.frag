#version 330 core

in vec3 out_local_pos;

layout(location = 0) out vec4 frag_color;

uniform samplerCube Skybox;

void main()
{
    vec3 env_color = texture(Skybox, out_local_pos).rgb;

    env_color = env_color / (env_color + vec3(1.0));
    env_color = pow(env_color, vec3(1.0 / 2.2));

    frag_color = vec4(env_color, 1.0);
}
