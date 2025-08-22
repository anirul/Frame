#version 450 core

in vec2 out_uv;

out vec4 frag_color;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 projection_inv;
uniform mat4 view_inv;
uniform mat4 model;
uniform vec3 camera_position;
// Direction from the light toward the scene.
uniform vec3 light_dir;
uniform vec3 light_color;
uniform sampler2D apple_texture;

struct Vertex
{
    vec3 position;
    float pad0;
    vec3 normal;
    float pad1;
    vec2 uv;
    vec2 pad2;
};

struct Triangle
{
    Vertex v0;
    Vertex v1;
    Vertex v2;
};

layout(std430, binding = 0) buffer TriangleBuffer
{
    Triangle triangles[];
};

bool rayTriangleIntersect(
    const vec3 ray_origin,
    const vec3 ray_direction,
    const Triangle triangle,
    out float out_t,
    out vec2 out_bary)
{
    const float EPSILON = 0.0000001;
    vec3 edge1 = triangle.v1.position - triangle.v0.position;
    vec3 edge2 = triangle.v2.position - triangle.v0.position;
    vec3 h = cross(ray_direction, edge2);
    float a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false; // Ray is parallel to triangle.
    float f = 1.0 / a;
    vec3 s = ray_origin - triangle.v0.position;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;
    vec3 q = cross(s, edge1);
    float v = f * dot(ray_direction, q);
    if (v < 0.0 || u + v > 1.0)
        return false;
    float t = f * dot(edge2, q);
    if (t > EPSILON)
    {
        out_t = t;
        out_bary = vec2(u, v);
        return true;
    }
    return false;
}

void main()
{
    // Reconstruct the view ray for this fragment.
    vec2 uv = out_uv * 2.0 - 1.0;
    vec4 clip_pos = vec4(uv, -1.0, 1.0);
    vec4 view_pos = projection_inv * clip_pos;
    view_pos = vec4(view_pos.xy, -1.0, 0.0);
    vec3 ray_dir_world = normalize((view_inv * view_pos).xyz);

    // Transform the ray to model space to match the triangle buffer
    mat4 inv_model = inverse(model);
    vec3 ray_origin = vec3(inv_model * vec4(camera_position, 1.0));
    vec3 ray_dir = normalize(mat3(inv_model) * ray_dir_world);

    // Trace the ray against all triangles and keep the closest hit.
    float closest_t = 1e20;
    vec3 hit_normal_model = vec3(0.0);
    vec2 hit_uv = vec2(0.0);
    bool hit = false;
    for (int i = 0; i < triangles.length(); ++i)
    {
        float t;
        vec2 bary;
        if (rayTriangleIntersect(ray_origin, ray_dir, triangles[i], t, bary) &&
            t < closest_t)
        {
            closest_t = t;
            float w = 1.0 - bary.x - bary.y;
            hit_normal_model = normalize(
                triangles[i].v0.normal * w +
                triangles[i].v1.normal * bary.x +
                triangles[i].v2.normal * bary.y);
            hit_uv = triangles[i].v0.uv * w + triangles[i].v1.uv * bary.x +
                     triangles[i].v2.uv * bary.y;
            hit = true;
        }
    }

    if (hit)
    {
        // Transform normal back to world space for lighting
        vec3 hit_normal =
            normalize(transpose(mat3(inv_model)) * hit_normal_model);

        // Fall back to a default light when no light uniforms are provided.
        vec3 dir = length(light_dir) > 0.0 ? light_dir : vec3(1.0, -1.0, -1.0);
        vec3 col = length(light_color) > 0.0 ? light_color : vec3(1.0);
        float diff = max(dot(hit_normal, normalize(-dir)), 0.0);
        vec3 tex_color = texture(apple_texture, hit_uv).rgb;
        frag_color = vec4(diff * col * tex_color, 1.0);
    }
    else
    {
        // Visualize the UV coordinates when no geometry is hit so we can
        // verify that the full-screen quad renders correctly.
        frag_color = vec4(out_uv, 0.0, 1.0);
    }
}
