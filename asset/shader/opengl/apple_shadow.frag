#version 450 core

in vec3 out_normal;
in vec3 out_world_position;
in vec2 out_uv;

uniform sampler2D apple_texture;
uniform vec3 light_pos;
uniform vec3 light_color;
uniform vec3 camera_pos;

out vec4 frag_color;

struct Triangle {
    vec3 v0;
    vec3 v1;
    vec3 v2;
};

layout(std430, binding = 0) buffer TriangleBuffer {
    Triangle triangles[];
};

// Moller-Trumbore intersection algorithm
bool rayTriangleIntersect(
    const vec3 ray_origin,
    const vec3 ray_direction,
    const Triangle triangle,
    out float out_t)
{
    const float EPSILON = 0.0000001;
    vec3 edge1 = triangle.v1 - triangle.v0;
    vec3 edge2 = triangle.v2 - triangle.v0;
    vec3 h = cross(ray_direction, edge2);
    float a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false;    // This ray is parallel to this triangle.
    float f = 1.0/a;
    vec3 s = ray_origin - triangle.v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;
    vec3 q = cross(s, edge1);
    float v = f * dot(ray_direction, q);
    if (v < 0.0 || u + v > 1.0)
        return false;
    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = f * dot(edge2, q);
    if (t > EPSILON) // ray intersection
    {
        out_t = t;
        return true;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return false;
}

void main() {
    // 1. Basic lighting (Lambert / Blinn-Phong, etc.)
    vec3 normal = normalize(out_normal);
    vec3 light_direction = normalize(light_pos - out_world_position);
    float diff = max(dot(normal, light_direction), 0.0);

    // 2. Compute shadow
    float shadow = 0.0;

    // 3. Combine
    vec3 texture_color = texture(apple_texture, out_uv).rgb;
    vec3 color = vec3(diff) * (1.0 - shadow) * light_color * texture_color;

    // For demonstration, let's just output that as final.
    frag_color = vec4(color, 1.0);
}
