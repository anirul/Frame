#version 330 core

in vec2 vert_texcoord;

layout(location = 0) out vec4 frag_color;

const vec2 resolution = vec2(640, 480);

// Get the color of the japanese flag at the coordonates given by uv.
// (uv is centered at the middle and uv.x is multiply by aspect ratio).
vec3 JapaneseFlag(vec2 uv)
{
	float l = length(uv);
	vec3 col = vec3(1, 1, 1);
	if (l < .3) col = vec3(1, 0, 0);
	return col;
}

void main()
{
	vec2 uv = vert_texcoord - vec2(0.5);
	uv.x *= resolution.x / resolution.y;

	vec3 color = JapaneseFlag(uv);
	frag_color = vec4(color, 1);
}