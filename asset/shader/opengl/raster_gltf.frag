#version 330 core

in vec3 vert_normal;
in vec2 vert_texcoord;
in vec3 vert_world_pos;

out vec4 frag_color;

uniform sampler2D albedo_texture;
uniform samplerCube skybox_env;
uniform sampler2D normal_texture;
uniform sampler2D roughness_texture;
uniform sampler2D metallic_texture;
uniform sampler2D ao_texture;
uniform sampler2D transmission_texture;
uniform sampler2D ior_texture;
uniform sampler2D thickness_texture;
uniform sampler2D attenuation_color_texture;
uniform sampler2D specular_factor_texture;
uniform sampler2D specular_color_texture;
uniform mat4 view;
uniform vec3 light_dir;
uniform int light_type;
uniform vec3 light_color;
uniform sampler2D shadow_map;
uniform mat4 light_view_projection;
uniform int shadow_enabled;
uniform float shadow_bias;
uniform float shadow_map_size;

const float pi = 3.14159265359;

vec3 apply_normal_map(vec3 normal)
{
    vec3 mapped = texture(normal_texture, vert_texcoord).xyz * 2.0 - 1.0;
    vec3 up = abs(normal.y) < 0.999 ? vec3(0.0, 1.0, 0.0)
                                    : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);
    return normalize(
        tangent * mapped.x + bitangent * mapped.y + normal * mapped.z);
}

float distribution_ggx(vec3 normal, vec3 halfway, float roughness)
{
    float alpha = max(roughness * roughness, 0.001);
    float alpha2 = alpha * alpha;
    float ndoth = max(dot(normal, halfway), 0.0);
    float ndoth2 = ndoth * ndoth;
    float denom = ndoth2 * (alpha2 - 1.0) + 1.0;
    return alpha2 / max(pi * denom * denom, 0.0001);
}

float geometry_schlick_ggx(float ndotv, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return ndotv / max(ndotv * (1.0 - k) + k, 0.0001);
}

float geometry_smith(vec3 normal, vec3 view, vec3 light, float roughness)
{
    float ndotv = max(dot(normal, view), 0.0);
    float ndotl = max(dot(normal, light), 0.0);
    return geometry_schlick_ggx(ndotv, roughness) *
           geometry_schlick_ggx(ndotl, roughness);
}

vec3 fresnel_schlick(float cos_theta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - clamp(cos_theta, 0.0, 1.0), 5.0);
}

vec3 fresnel_schlick_roughness(float cos_theta, vec3 f0, float roughness)
{
    vec3 rough_f0 = max(vec3(1.0 - roughness), f0);
    return f0 + (rough_f0 - f0) *
                    pow(1.0 - clamp(cos_theta, 0.0, 1.0), 5.0);
}

float sample_shadow(vec3 normal, vec3 light)
{
    if (shadow_enabled == 0)
    {
        return 1.0;
    }

    vec4 light_clip = light_view_projection * vec4(vert_world_pos, 1.0);
    if (abs(light_clip.w) < 0.0001)
    {
        return 1.0;
    }
    vec3 light_ndc = light_clip.xyz / light_clip.w;
    vec2 shadow_uv = light_ndc.xy * 0.5 + 0.5;
    float current_depth = light_ndc.z * 0.5 + 0.5;
    if (shadow_uv.x < 0.0 || shadow_uv.x > 1.0 || shadow_uv.y < 0.0 ||
        shadow_uv.y > 1.0 || current_depth < 0.0 || current_depth > 1.0)
    {
        return 1.0;
    }

    float bias = max(shadow_bias * (1.0 - dot(normal, light)), 0.0008);
    vec2 texel_size = vec2(1.0 / max(shadow_map_size, 1.0));
    float visibility = 0.0;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float closest_depth =
                texture(shadow_map, shadow_uv + vec2(x, y) * texel_size).r;
            visibility += current_depth - bias <= closest_depth ? 1.0 : 0.25;
        }
    }
    return visibility / 9.0;
}

void main()
{
    vec3 normal = apply_normal_map(normalize(vert_normal));
    vec3 camera_pos = vec3(inverse(view)[3]);
    vec3 view_dir = normalize(camera_pos - vert_world_pos);
    vec3 reflected_dir = reflect(-view_dir, normal);

    vec3 albedo = texture(albedo_texture, vert_texcoord).rgb;
    float roughness =
        clamp(texture(roughness_texture, vert_texcoord).r, 0.04, 1.0);
    float metallic =
        clamp(texture(metallic_texture, vert_texcoord).r, 0.0, 1.0);
    float ao = clamp(texture(ao_texture, vert_texcoord).r, 0.0, 1.0);
    float transmission =
        clamp(texture(transmission_texture, vert_texcoord).r, 0.0, 1.0);
    float ior = max(texture(ior_texture, vert_texcoord).r, 1.01);
    float thickness =
        clamp(texture(thickness_texture, vert_texcoord).r, 0.0, 1.0);
    vec3 attenuation = texture(attenuation_color_texture, vert_texcoord).rgb;
    float specular_factor =
        clamp(texture(specular_factor_texture, vert_texcoord).a, 0.0, 1.0);
    vec3 specular_tint =
        clamp(texture(specular_color_texture, vert_texcoord).rgb, 0.0, 1.0);

    vec3 light = length(light_dir) > 0.0
        ? normalize(-light_dir)
        : normalize(vec3(-0.4, -0.7, 0.6));
    vec3 halfway = normalize(view_dir + light);
    float ndotv = max(dot(normal, view_dir), 0.0);
    float ndotl = max(dot(normal, light), 0.0);
    float shadow = sample_shadow(normal, light);
    vec3 env_reflection = texture(skybox_env, reflected_dir).rgb;
    vec3 dielectric_f0 = vec3(0.04) * specular_factor * specular_tint;
    vec3 f0 = mix(dielectric_f0, albedo, metallic);

    float normal_distribution = distribution_ggx(normal, halfway, roughness);
    float geometry = geometry_smith(normal, view_dir, light, roughness);
    vec3 fresnel = fresnel_schlick(max(dot(halfway, view_dir), 0.0), f0);
    vec3 specular_brdf =
        normal_distribution * geometry * fresnel /
        max(4.0 * ndotv * ndotl, 0.0001);
    vec3 diffuse_weight = (vec3(1.0) - fresnel) * (1.0 - metallic);
    vec3 direct =
        (diffuse_weight * albedo / pi + specular_brdf) *
        max(light_color, vec3(0.0)) *
        ndotl *
        shadow;

    vec3 env_diffuse = texture(skybox_env, normal).rgb * albedo;
    vec3 env_fresnel = fresnel_schlick_roughness(ndotv, f0, roughness);
    vec3 env_diffuse_weight = (vec3(1.0) - env_fresnel) * (1.0 - metallic);
    vec3 env_specular =
        env_reflection * env_fresnel * mix(1.0, 0.18, roughness);
    vec3 ambient = (env_diffuse_weight * env_diffuse * 0.35 + env_specular) * ao;
    vec3 surface_color = direct + ambient;

    vec3 refracted_dir = refract(-view_dir, normal, 1.0 / ior);
    if (length(refracted_dir) < 0.001)
    {
        refracted_dir = reflected_dir;
    }
    vec3 env_refraction = texture(skybox_env, refracted_dir).rgb;
    float transmission_fresnel = max(max(env_fresnel.r, env_fresnel.g), env_fresnel.b);
    vec3 transmission_color =
        mix(env_refraction * albedo * attenuation, env_reflection, transmission_fresnel);
    transmission_color = mix(transmission_color, env_reflection, thickness);

    vec3 color = mix(surface_color, transmission_color, transmission);
    frag_color = vec4(color, 1.0);
}
