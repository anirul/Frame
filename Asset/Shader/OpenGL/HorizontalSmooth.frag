#version 330 core
// This shader will scan horizontally for changes in depth value.

in vec2 vert_texcoord;
layout(location = 0) out vec4 frag_color;

uniform float time_s;
uniform sampler2D Depth;

vec2 resolution = vec2(textureSize(Depth, 0));
// Considering that the screen at the person distance is more or less 1m2.
// This will compute the move per meter on the pixel level.
vec2 one_meter_on_resolution = 1.0 / resolution;

float GetValueAt(vec2 uv)
{
	return texture(Depth, uv).r;
}

float ComputeValue(vec2 uv) {
	float left = GetValueAt(vec2(uv.x - one_meter_on_resolution.x, uv.y));
	float first = GetValueAt(uv);
	float second = GetValueAt(vec2(uv.x + one_meter_on_resolution.x, uv.y));
	float third = GetValueAt(vec2(uv.x + 2 * one_meter_on_resolution.x, uv.y));
	if (first == second && 
		left != second && 
		second != 0.0)
	{
		return 0.0;
	}
	return first;
}

void main()
{
	frag_color = vec4(vec3(ComputeValue(vert_texcoord)), 1);
	// Store in normal map format (moved to [0, 1] scale).
}
