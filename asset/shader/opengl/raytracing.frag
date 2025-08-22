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
    // Reconstruct the view ray for this fragment.
    vec2 uv = out_uv * 2.0 - 1.0;
    vec4 clip_pos = vec4(uv, -1.0, 1.0);
    vec4 view_pos = inverse(projection) * clip_pos;
    view_pos = vec4(view_pos.xy, -1.0, 0.0);
    vec3 ray_dir = normalize((inverse(view) * view_pos).xyz);

    // Trace the ray against all triangles and keep the closest hit.
    float closest_t = 1e20;
    vec3 hit_normal = vec3(0.0);
    bool hit = false;
    for (int i = 0; i < triangles.length(); ++i)
    {
        float t;
        if (rayTriangleIntersect(camera_position, ray_dir, triangles[i], t) &&
            t < closest_t)
        {
            closest_t = t;
            vec3 edge1 = triangles[i].v1 - triangles[i].v0;
            vec3 edge2 = triangles[i].v2 - triangles[i].v0;
            hit_normal = normalize(cross(edge1, edge2));
            hit = true;
        }
    }

    if (hit)
    {
        // Fall back to a default light when no light uniforms are provided.
        vec3 dir = length(light_dir) > 0.0 ? light_dir : vec3(1.0, -1.0, -1.0);
        vec3 col = length(light_color) > 0.0 ? light_color : vec3(1.0);
        float diff = max(dot(hit_normal, normalize(-dir)), 0.0);
        frag_color = vec4(diff * col, 1.0);
    }
    else
    {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
