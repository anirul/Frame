#version 330 core

in vec3 out_normal;
in vec2 out_texcoord;

layout(location = 0) out vec4 frag_color;

uniform float Time;

const vec2 resolution = vec2(640, 480);
const int max_steps = 200;
const float min_dist = 0.01;
const float max_dist = 100.;

// Get the distance and normal to the surface
// (if the distance is < min_dist in w).
vec4 GetDist(vec3 position)
{
	vec4 sphere = vec4(0, 1, 6, 1);
	float dist_sphere = length(position - sphere.xyz) - sphere.w;
	float dist_plane = position.y;
	if (dist_sphere < dist_plane)
		return vec4(normalize(position - sphere.xyz), dist_sphere);
	else
		return vec4(0, 1, 0, dist_plane);
}

// Get the new distance and the normal to the surface.
// (if the distance is < min_dist in w).
vec4 RayMarching(vec3 ray_origin, vec3 ray_direction)
{
	float dist0 = 0;
	for (int i = 0; i < max_steps; ++i)
	{
		vec3 p = ray_origin + ray_direction * dist0;
		vec4 normal_dist = GetDist(p);
		dist0 += normal_dist.w;
		if (normal_dist.w < min_dist || dist0 > max_dist)
			return vec4(normal_dist.xyz, dist0);
	}
	return vec4(0, 1, 0, dist0);
}

// Get the light position at time.
vec3 LightPosition()
{
	vec3 light_position = vec3(0, 5, 6);
	light_position.xz += vec2(sin(Time), cos(Time) * 2);
	return light_position;
}

// Get the light normal and the light value.
vec4 LightNormalValue(vec3 position, vec3 normal)
{
	vec3 light_position = LightPosition();
	vec3 light_normal = normalize(light_position - position);
	float light_value = dot(normal, light_normal);
	return vec4(light_normal.xyz, light_value);
}

// Get light without shadow.
float LightOnly(vec3 position, vec3 normal)
{
	return LightNormalValue(position, normal).w;
}

// Calculate the Shadow and light.
float LightAndShadow(vec3 position, vec3 normal)
{
	vec3 light_position = LightPosition();
	vec4 light_normal_value = LightNormalValue(position, normal);
	float dist_light = 
		RayMarching(position + normal * min_dist * 2, light_normal_value.xyz).w;
	if (dist_light < length(light_position - position)) 
		light_normal_value.w *= .1;
	return light_normal_value.w;
}

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
	vec2 uv = out_texcoord - vec2(0.5);
	uv.x *= resolution.x / resolution.y;
	uv.y = -uv.y;
	
	vec3 ray_origin = vec3(0, 1, 0);
	vec3 ray_direction = normalize(vec3(uv.x, uv.y, 1));
	vec4 result = RayMarching(ray_origin, ray_direction);
	vec3 position = ray_origin + ray_direction * result.w;
	float light = LightOnly(position, result.xyz);
	float light_shadow = LightAndShadow(position, result.xyz);

	// vec3 color = JapaneseFlag(uv);
	// vec3 color = vec3(result.w / 8, result.w / 4, result.w);
	// vec3 color = vec3(light);
	vec3 color = vec3(light_shadow);
	frag_color = vec4(color, 1);
}