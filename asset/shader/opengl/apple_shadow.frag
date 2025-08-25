#version 450 core

in vec3 out_normal;
in vec3 out_world_position;
in vec2 out_uv;

uniform sampler2D apple_texture;
// Direction from the light toward the scene.
uniform vec3 light_dir;
uniform vec3 light_color;
uniform mat4 model;

out vec4 frag_color;

struct Vertex {
    vec3 position;
    float pad0;
    vec3 normal;
    float pad1;
    vec2 uv;
    vec2 pad2;
};

struct Triangle {
    Vertex v0;
    Vertex v1;
    Vertex v2;
};

layout(std430, binding = 0) buffer TriangleBuffer {
    Triangle triangles[];
};

struct BvhNode {
    vec3 min;
    float pad0;
    vec3 max;
    float pad1;
    int left;
    int right;
    int first_triangle;
    int triangle_count;
};

layout(std430, binding = 1) buffer BvhBuffer {
    BvhNode nodes[];
};

// Moller-Trumbore intersection algorithm
bool rayTriangleIntersect(
    const vec3 ray_origin,
    const vec3 ray_direction,
    const Triangle triangle,
    out float out_t)
{
    const float EPSILON = 0.0000001;
    vec3 edge1 = triangle.v1.position - triangle.v0.position;
    vec3 edge2 = triangle.v2.position - triangle.v0.position;
    vec3 h = cross(ray_direction, edge2);
    float a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false;    // This ray is parallel to this triangle.
    float f = 1.0/a;
    vec3 s = ray_origin - triangle.v0.position;
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

bool rayAabbIntersect(const vec3 ray_origin, const vec3 ray_dir, const BvhNode node)
{
    vec3 inv_dir = 1.0 / ray_dir;
    vec3 t0 = (node.min - ray_origin) * inv_dir;
    vec3 t1 = (node.max - ray_origin) * inv_dir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float t_enter = max(max(tmin.x, tmin.y), tmin.z);
    float t_exit = min(min(tmax.x, tmax.y), tmax.z);
    return t_exit >= max(t_enter, 0.0);
}

bool anyHitBVH(const vec3 ray_origin, const vec3 ray_dir)
{
    int stack[64];
    int stack_ptr = 0;
    stack[stack_ptr++] = 0;
    while (stack_ptr > 0)
    {
        int node_index = stack[--stack_ptr];
        BvhNode node = nodes[node_index];
        if (!rayAabbIntersect(ray_origin, ray_dir, node))
            continue;
        if (node.triangle_count > 0)
        {
            for (int i = 0; i < node.triangle_count; ++i)
            {
                int tri_index = node.first_triangle + i;
                float t;
                if (rayTriangleIntersect(ray_origin, ray_dir, triangles[tri_index], t))
                    return true;
            }
        }
        else
        {
            if (node.left >= 0)
                stack[stack_ptr++] = node.left;
            if (node.right >= 0)
                stack[stack_ptr++] = node.right;
        }
    }
    return false;
}

void main() {
    // 1. Basic lighting (Lambert / Blinn-Phong, etc.)
    vec3 normal = normalize(out_normal);
    // Light direction points from the fragment toward the light source (world space).
    vec3 light_direction_world = normalize(-light_dir);
    float diff = max(dot(normal, light_direction_world), 0.0);

    // 2. Compute shadow
    float shadow = 0.0;
    vec3 ray_origin_world = out_world_position + normal * 0.001;
    mat4 inv_model = inverse(model);
    vec3 ray_origin = vec3(inv_model * vec4(ray_origin_world, 1.0));
    vec3 light_direction = normalize(mat3(inv_model) * light_direction_world);
    if (anyHitBVH(ray_origin, light_direction))
        shadow = 1.0;

    // 3. Combine
    vec3 texture_color = texture(apple_texture, out_uv).rgb;
    vec3 color = vec3(diff) * (1.0 - shadow) * light_color * texture_color;

    // For demonstration, let's just output that as final.
    frag_color = vec4(color, 1.0);
}
