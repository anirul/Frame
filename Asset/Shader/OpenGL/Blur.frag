#version 330 core

in vec2 vert_texcoord;
layout (location = 0) out vec4 frag_color;

// uniform float exponent;
uniform sampler2D Image;

const float exponent = 1.0;
const int start_stop_value = 3;

void main() 
{
	vec2 texel_size = 1.0 / vec2(textureSize(Image, 0));
	vec3 result = vec3(0.0);
	for (int x = -start_stop_value; x < start_stop_value; ++x)
	{
		for (int y = -start_stop_value; y < start_stop_value; ++y)
		{
			vec2 offset = vec2(float(x), float(y)) * texel_size;
			result += texture(Image, vert_texcoord + offset).rgb;
		}
	}
	result /= (start_stop_value * 2) * (start_stop_value * 2);
	// Check that the value is > 0 to avoid buring in the pow.
	result = max(vec3(0.0), result);
	result = pow(result, vec3(exponent));
	result = min(vec3(1.0), result);
	frag_color = vec4(result, 1.0);
}
