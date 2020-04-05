#version 330 core
out vec4 frag_color;

in vec3 out_texcoord;

uniform samplerCube Skybox;

void main()
{    
    frag_color = texture(Skybox, out_texcoord);
}