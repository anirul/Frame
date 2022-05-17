#version 330 core
// DEPTH_TO_X_Y is a value to multiply the depth to be = to the left / right.
// Ok at the person level 1 pixel is more or less equal to 1mm (0.001m).
// and the depth buffer is computed in mm (1.1mm).
// now we also have the fact that the texture is in short (unsigned short).
// but loaded as a signed short (this should not be a probleme as long as we
// don't reach the last bit mean more than 32m).
// so the depth is = to depth * 32767.
// and this should be divided by 1000 (to be in m).
#define DEPTH_TO_METER (32767.0 / 1000.0)

in vec2 vert_texcoord;
layout(location = 0) out vec4 frag_color;

uniform float time_s;
uniform sampler2D Depth;

const vec2 resolution = vec2(1280, 960);
// considering that the screen at the person distance is more or less 1m2.
const vec2 one_meter_on_resolution = 1.0 / resolution;

float GetValueAt(vec2 uv)
{
	return texture(Depth, uv).r * DEPTH_TO_METER;
}

vec3 ComputeNormal(vec2 uv)
{
	// Get the coordinates of the point around the center.
	vec3 coord_up = vec3(uv.x + one_meter_on_resolution.x, uv.y, 0);
	vec3 coord_down = vec3(uv.x - one_meter_on_resolution.x, uv.y, 0);
	vec3 coord_left = vec3(uv.x, uv.y - one_meter_on_resolution.y, 0);
	vec3 coord_right = vec3(uv.x, uv.y + one_meter_on_resolution.y, 0);
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
	return normalize(first_vec + second_vec);
}

void main()
{
	vec3 normal = ComputeNormal(vert_texcoord);
	normal = normal * 0.5 + 0.5;
	frag_color = vec4(normal, 1);
}
