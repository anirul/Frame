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
uniform sampler2D apple_normal_texture;
uniform sampler2D apple_roughness_texture;
uniform sampler2D apple_metalness_texture;
uniform sampler2D apple_ao_texture;
uniform samplerCube skybox_env;

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

struct BvhNode
{
    vec3 min;
    float pad0;
    vec3 max;
    float pad1;
    int left;
    int right;
    int first_triangle;
    int triangle_count;
};

layout(std430, binding = 0) buffer TriangleBuffer
{
    Triangle triangles[];
};

layout(std430, binding = 1) buffer BvhBuffer
{
    BvhNode nodes[];
};

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (3.14159265 * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

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

bool rayAabbIntersect(
    const vec3 ray_origin, const vec3 ray_dir, const BvhNode node)
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

bool traverseBVH(
    const vec3 ray_origin,
    const vec3 ray_dir,
    out float out_t,
    out vec2 out_bary,
    out int out_tri)
{
    int stack[64];
    int stack_ptr = 0;
    stack[stack_ptr++] = 0;
    out_t = 1e20;
    out_tri = -1;
    bool hit = false;
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
                vec2 bary;
                if (rayTriangleIntersect(
                        ray_origin,
                        ray_dir,
                        triangles[tri_index],
                        t,
                        bary) &&
                    t < out_t)
                {
                    out_t = t;
                    out_bary = bary;
                    out_tri = tri_index;
                    hit = true;
                }
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
    return hit;
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
                vec2 bary;
                if (rayTriangleIntersect(
                        ray_origin,
                        ray_dir,
                        triangles[tri_index],
                        t,
                        bary))
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

    // Trace the ray against the BVH.
    float closest_t;
    vec2 hit_bary;
    int tri_index;
    bool hit = traverseBVH(ray_origin, ray_dir, closest_t, hit_bary, tri_index);
    vec3 hit_normal_model = vec3(0.0);
    vec2 hit_uv = vec2(0.0);
    vec3 hit_tangent_model = vec3(0.0);
    vec3 hit_bitangent_model = vec3(0.0);
    if (hit)
    {
        Triangle tri = triangles[tri_index];
        float w = 1.0 - hit_bary.x - hit_bary.y;
        hit_normal_model = normalize(
            tri.v0.normal * w +
            tri.v1.normal * hit_bary.x +
            tri.v2.normal * hit_bary.y);
        hit_uv = tri.v0.uv * w + tri.v1.uv * hit_bary.x +
                 tri.v2.uv * hit_bary.y;
        vec3 edge1 = tri.v1.position - tri.v0.position;
        vec3 edge2 = tri.v2.position - tri.v0.position;
        vec2 deltaUV1 = tri.v1.uv - tri.v0.uv;
        vec2 deltaUV2 = tri.v2.uv - tri.v0.uv;
        float f = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        if (!isinf(f))
        {
            hit_tangent_model =
                normalize(f * (deltaUV2.y * edge1 - deltaUV1.y * edge2));
            hit_bitangent_model =
                normalize(f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2));
        }
    }

    if (hit)
    {
        // Transform to world space
        mat3 inv_model3 = transpose(mat3(inv_model));
        vec3 N = normalize(inv_model3 * hit_normal_model);
        vec3 T = normalize(inv_model3 * hit_tangent_model);
        vec3 B = normalize(inv_model3 * hit_bitangent_model);
        vec3 normal_map = texture(apple_normal_texture, hit_uv).xyz * 2.0 - 1.0;
        vec3 hit_normal = normalize(mat3(T, B, N) * normal_map);
        // Position of the hit point in model space for casting shadow rays.
        vec3 hit_pos_model = ray_origin + closest_t * ray_dir;

        // Fall back to a default light when no light uniforms are provided.
        vec3 dir = length(light_dir) > 0.0 ? light_dir : vec3(1.0, -1.0, 1.0);
        vec3 col = length(light_color) > 0.0 ? light_color : vec3(1.0);

        // Cast a shadow ray toward the light in model space.
        vec3 shadow_dir = normalize(mat3(inv_model) * -dir);
        vec3 shadow_origin = hit_pos_model + hit_normal_model * 0.0001;
        bool in_shadow = anyHitBVH(shadow_origin, shadow_dir);

        float shadow_factor = in_shadow ? 0.3 : 1.0;
        vec3 albedo = texture(apple_texture, hit_uv).rgb;
        float roughness = texture(apple_roughness_texture, hit_uv).r;
        float metallic = texture(apple_metalness_texture, hit_uv).r;
        float ao = texture(apple_ao_texture, hit_uv).r;
        vec3 V = normalize(camera_position - (model * vec4(hit_pos_model, 1.0)).xyz);
        vec3 L = normalize(-dir);
        vec3 H = normalize(V + L);
        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        float D = DistributionGGX(hit_normal, H, roughness);
        float G = GeometrySmith(hit_normal, V, L, roughness);
        vec3 numerator = D * G * F;
        float denominator = 4.0 * max(dot(hit_normal, V), 0.0) * max(dot(hit_normal, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        vec3 diffuse = kD * albedo / 3.14159265;
        float NdotL = max(dot(hit_normal, L), 0.0);
        vec3 env_reflect_dir = reflect(-V, hit_normal);
        env_reflect_dir = vec3(env_reflect_dir.x, -env_reflect_dir.y, env_reflect_dir.z);
        vec3 env_color = texture(skybox_env, env_reflect_dir).rgb;
        vec3 ambient = kD * albedo * 0.25 * ao +
                       kD * albedo / 3.14159265 * env_color * ao;
        vec3 env_specular = specular * env_color * ao * (in_shadow ? 0.0 : 1.0);
        vec3 Lo = diffuse * col * NdotL * shadow_factor +
                  specular * col * NdotL * (in_shadow ? 0.0 : 1.0) +
                  env_specular;
        frag_color = vec4(ambient + Lo, 1.0);
    }
    else
    {
        discard;
    }
}
