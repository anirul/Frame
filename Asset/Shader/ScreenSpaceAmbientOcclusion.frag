#version 330 core

in vec2 vert_texcoord;

layout (location = 0) out vec4 frag_color;

uniform sampler2D ViewPosition;
uniform sampler2D ViewNormal;
uniform sampler2D Noise;

uniform vec3 kernel[64];
uniform vec2 noise_scale;
uniform mat4 projection;

const int kernel_size = 64;
const float radius = 1;
const float bias = 0.0025;

void main()
{
	// get input for SSAO algorithm
    vec3 position = texture(ViewPosition, vert_texcoord).xyz;
    vec3 normal = normalize(texture(ViewNormal, vert_texcoord).rgb);
    vec3 random = normalize(texture(Noise, vert_texcoord * noise_scale).xyz);

    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(random - normal * dot(random, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 kernel_matrix = mat3(tangent, bitangent, normal);

    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0;

    for(int i = 0; i < kernel_size; ++i)
    {
        // Get sample position.
        vec3 sample_view = kernel_matrix * kernel[i];
        vec3 sample_position = position + sample_view * radius;
        vec2 sample_texcoord = vert_texcoord + sample_view.xy * radius;

        float depth = texture(ViewPosition, sample_texcoord).z;
        float range = smoothstep(0, 1, radius / abs(position.z - depth));
        occlusion += ((depth >= sample_position.z + bias) ? 1.0 : 0.0) * range;
    }

    occlusion = 1.0 - occlusion / (kernel_size - 1.0);

    frag_color = vec4(vec3(occlusion), 1.0);
}
