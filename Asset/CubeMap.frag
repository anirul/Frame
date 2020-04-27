#version 330 core

in vec3 vert_local_pos;

layout(location = 0) out vec4 frag_color;

uniform samplerCube Skybox;

void main()
{
    frag_color = vec4(texture(Skybox, vert_local_pos).rgb, 1.0);
}
