#version 330 core
// MULTIPLE_VALUE is a value to multiply the depth to be = to the left / right.
// The value is set to a value by look so not really good.
#define MULTIPLE_VALUE 256

in vec2 vert_texcoord;

layout(location = 0) out vec4 frag_color;

uniform sampler2D Depth;

const vec2 resolution = vec2(1280, 960);
const vec2 one_on_half_resolution = 1 / (resolution / 2);

float GetValueAt(vec2 uv)
{
	return texture(Depth, uv).r * MULTIPLE_VALUE;
}

vec3 ComputeNormal(vec2 uv)
{
	// Get the coordinates of the point around the center.
	vec3 coord_up = vec3(uv.x + one_on_half_resolution.x, uv.y, 0);
	vec3 coord_down = vec3(uv.x - one_on_half_resolution.x, uv.y, 0);
	vec3 coord_left = vec3(uv.x, uv.y - one_on_half_resolution.y, 0);
	vec3 coord_right = vec3(uv.x, uv.y + one_on_half_resolution.y, 0);
	vec3 coord_center = vec3(uv.x, uv.y, 0);
	// Correct the Z value to be = to the depth value.
	coord_up.z = GetValueAt(vec2(coord_up.x, coord_up.y));
	coord_down.z = GetValueAt(vec2(coord_down.x, coord_down.y));
	coord_left.z = GetValueAt(vec2(coord_left.x, coord_left.y));
	coord_right.z = GetValueAt(vec2(coord_right.x, coord_right.y));
	coord_center.z = GetValueAt(vec2(coord_center.x, coord_center.y));
	// This should be correct in a right system.
	vec3 first_vec = 
		cross(coord_up - coord_center, coord_left - coord_center);
	vec3 second_vec = 
		cross(coord_down - coord_center, coord_right - coord_center);
	// Why? probably some inverse due to Z beeing forward?
	first_vec.z = abs(first_vec.z);
	second_vec.z = abs(second_vec.z);
	vec3 normal = normalize(first_vec + second_vec);
	normal = normal * 0.5 + 0.5;
	return normal;
}

void main()
{
	frag_color = vec4(ComputeNormal(vert_texcoord), 1);
}
