#version 330 core

in vec2 vert_texcoord;
in vec3 vert_position;
in vec3 vert_normal;

layout (location = 0) out vec4 frag_position;
layout (location = 1) out vec4 frag_normal;
layout (location = 2) out vec4 frag_albedo;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    frag_position = vec4(vert_position, 1.0);
    // also store the per-fragment normals into the gbuffer
    frag_normal = vec4(normalize(vert_normal), 1.0);
    // and the diffuse per-fragment color
    frag_albedo = vec4(0.95, 0.95, 0.95, 1.0);
}