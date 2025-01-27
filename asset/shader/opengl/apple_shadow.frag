#version 450 core

in vec3 out_normal;
in vec3 out_world_position;
in vec2 out_uv;
in vec4 out_light_space_pos;

uniform sampler2D shadow_map;
uniform sampler2D apple_texture;
uniform vec3 light_pos;
uniform vec3 light_color;
uniform vec3 camera_pos;
uniform float bias;

out vec4 frag_color;

void main() {
    // 1. Basic lighting (Lambert / Blinn-Phong, etc.)
    vec3 normal = normalize(out_normal);
    vec3 light_direction = normalize(light_pos - out_world_position);
    float diff = max(dot(normal, light_direction), 0.0);

    // 2. Compute shadow
    // Transform from clip-space [-1..1] to [0..1] coordinates
    vec3 proj_coords = out_light_space_pos.xyz / out_light_space_pos.w;
    proj_coords = proj_coords * 0.5 + 0.5;

    // If outside the [0,1] range, skip shadow or clamp
    // (for directional light you might clamp, or check with an if)
    if (proj_coords.x < 0.0 || proj_coords.x > 1.0 ||
        proj_coords.y < 0.0 || proj_coords.y > 1.0 ||
        proj_coords.z < 0.0 || proj_coords.z > 1.0)
    {
        // Fragment is outside the shadow map area
        // so for directional lights, typically no shadow 
        // (or 1 if you want to treat it as shadow)
    }

    float current_depth = proj_coords.z;
    float closest_depth = texture(shadow_map, proj_coords.xy).r;
    // Subtract a small bias to reduce shadow acne
    float shadow = current_depth - bias > closest_depth ? 1.0 : 0.0;

    // 3. Combine
    vec3 color = vec3(diff) * (1.0 - shadow) * light_color;

    // For demonstration, let's just output that as final.
    frag_color = vec4(color, 1.0);
}
