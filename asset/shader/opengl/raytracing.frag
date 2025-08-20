#version 450 core

in vec2 out_uv;

out vec4 frag_color;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 camera_position;
// Direction from the light toward the scene.
uniform vec3 light_dir;
uniform vec3 light_color;

struct Triangle {
    vec3 v0;
    vec3 v1;
    vec3 v2;
};

layout(std430, binding = 0) buffer TriangleBuffer {
    Triangle triangles[];
};

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
        return false;    // Ray is parallel to triangle.
    float f = 1.0 / a;
    vec3 s = ray_origin - triangle.v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;
    vec3 q = cross(s, edge1);
    float v = f * dot(ray_direction, q);
    if (v < 0.0 || u + v > 1.0)
        return false;
    float t = f * dot(edge2, q);
    if (t > EPSILON) {
        out_t = t;
        return true;
    }
    return false;
}

void main()
{
    vec2 ndc = out_uv * 2.0 - 1.0;
    // Reconstruct the view-space ray and transform it to world space.
    vec4 ray_clip = vec4(ndc, -1.0, 1.0);
    vec4 ray_eye = inverse(projection) * ray_clip;
    ray_eye = vec4(ray_eye.xy, -1.0, 0.0);
    vec3 ray_dir_world = normalize((inverse(view) * ray_eye).xyz);

    float t_min = 1e30;
    vec3 hit_normal = vec3(0.0);
    bool hit = false;
    for (int i = 0; i < triangles.length(); ++i) {
        float t;
        if (rayTriangleIntersect(camera_position, ray_dir_world, triangles[i], t)) {
            if (t < t_min) {
                t_min = t;
                vec3 v0 = triangles[i].v0;
                vec3 v1 = triangles[i].v1;
                vec3 v2 = triangles[i].v2;
                hit_normal = normalize(cross(v1 - v0, v2 - v0));
                hit = true;
            }
        }
    }

    if (hit) {
        // For debugging, shade intersections as solid red to verify
        // that the ray/triangle tests work before adding lighting.
        frag_color = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
