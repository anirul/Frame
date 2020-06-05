#version 330 core

in vec2 vert_texcoord;

layout (location = 0) out vec4 frag_color;

uniform sampler2D Ambient;
uniform sampler2D Normal;
uniform sampler2D MetalRoughAO;
uniform sampler2D Position;

void main()
{
	frag_color = vec4(1.0, 1.0, 1.0, 1.0);
}