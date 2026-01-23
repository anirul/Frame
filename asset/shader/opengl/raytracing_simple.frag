#version 450 core

in vec2 out_uv;

out vec4 frag_color;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 projection_inv;
uniform mat4 view_inv;
uniform mat4 model;
uniform mat4 model_inv;
uniform vec3 camera_position;
uniform mat4 env_map_model;
// Direction from the light toward the scene.
uniform vec3 light_dir;
uniform vec3 light_color;
// Generic PBR textures for the traced mesh.
uniform sampler2D albedo_texture;
uniform sampler2D normal_texture;
uniform sampler2D roughness_texture;
uniform sampler2D metallic_texture;
uniform sampler2D ao_texture;
uniform samplerCube skybox;
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

layout(std430, binding = 0) buffer TriangleBufferGlass
{
    Triangle glass_triangles[];
};

layout(std430, binding = 1) buffer TriangleBufferGround
{
    Triangle ground_triangles[];
};

const int kMaterialGlass = 0;
const int kMaterialGround = 1;

int TriangleCountGlass()
{
    return int(glass_triangles.length());
}

int TriangleCountGround()
{
    return int(ground_triangles.length());
}

struct HitInfo
{
    bool hit;
    float t;
    vec2 bary;
    int tri_index;
    int material_id;
    vec3 pos_model;
    vec3 normal_model;
    vec3 tangent_model;
    vec3 bitangent_model;
    vec2 uv;
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

float fresnelDielectric(float cosTheta, float ior)
{
    float r0 = (ior - 1.0) / (ior + 1.0);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0);
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

bool anyHitTriangles(const vec3 ray_origin, const vec3 ray_dir)
{
    int tri_count = TriangleCountGlass();
    for (int i = 0; i < tri_count; ++i)
    {
        float t;
        vec2 bary;
        if (rayTriangleIntersect(
                ray_origin,
                ray_dir,
                glass_triangles[i],
                t,
                bary))
            return true;
    }
    int ground_count = TriangleCountGround();
    for (int i = 0; i < ground_count; ++i)
    {
        float t;
        vec2 bary;
        if (rayTriangleIntersect(
                ray_origin,
                ray_dir,
                ground_triangles[i],
                t,
                bary))
            return true;
    }
    return false;
}

HitInfo TraceScene(const vec3 ray_origin, const vec3 ray_dir)
{
    HitInfo info;
    info.hit = false;
    info.t = 0.0;
    info.bary = vec2(0.0);
    info.tri_index = -1;
    info.material_id = -1;
    info.pos_model = vec3(0.0);
    info.normal_model = vec3(0.0);
    info.tangent_model = vec3(0.0);
    info.bitangent_model = vec3(0.0);
    info.uv = vec2(0.0);

    float best_t = 1e20;
    vec2 best_bary = vec2(0.0);
    int best_tri = -1;
    int best_material = -1;
    int tri_count = TriangleCountGlass();
    for (int i = 0; i < tri_count; ++i)
    {
        float t;
        vec2 bary;
        if (rayTriangleIntersect(ray_origin, ray_dir, glass_triangles[i], t, bary) &&
            t < best_t)
        {
            best_t = t;
            best_bary = bary;
            best_tri = i;
            best_material = kMaterialGlass;
        }
    }
    int ground_count = TriangleCountGround();
    for (int i = 0; i < ground_count; ++i)
    {
        float t;
        vec2 bary;
        if (rayTriangleIntersect(ray_origin, ray_dir, ground_triangles[i], t, bary) &&
            t < best_t)
        {
            best_t = t;
            best_bary = bary;
            best_tri = i;
            best_material = kMaterialGround;
        }
    }
    if (best_tri < 0)
    {
        return info;
    }

    info.hit = true;
    info.t = best_t;
    info.bary = best_bary;
    info.tri_index = best_tri;
    info.material_id = best_material;
    info.pos_model = ray_origin + best_t * ray_dir;

    Triangle tri = (best_material == kMaterialGround)
        ? ground_triangles[best_tri]
        : glass_triangles[best_tri];
    float w = 1.0 - best_bary.x - best_bary.y;
    info.normal_model = normalize(
        tri.v0.normal * w +
        tri.v1.normal * best_bary.x +
        tri.v2.normal * best_bary.y);
    info.uv = tri.v0.uv * w + tri.v1.uv * best_bary.x +
              tri.v2.uv * best_bary.y;

    vec3 edge1 = tri.v1.position - tri.v0.position;
    vec3 edge2 = tri.v2.position - tri.v0.position;
    vec2 deltaUV1 = tri.v1.uv - tri.v0.uv;
    vec2 deltaUV2 = tri.v2.uv - tri.v0.uv;
    float f = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
    if (!isinf(f))
    {
        info.tangent_model =
            normalize(f * (deltaUV2.y * edge1 - deltaUV1.y * edge2));
        info.bitangent_model =
            normalize(f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2));
    }
    return info;
}

bool IsGround(const HitInfo hit)
{
    return hit.material_id == kMaterialGround;
}

vec3 SampleEnvSpecular(
    vec3 dir_world,
    mat3 env_rot_inv,
    float roughness,
    float max_env_lod)
{
    float specular_lod = clamp(roughness * max_env_lod, 0.0, max_env_lod);
    vec3 env_dir = env_rot_inv * dir_world;
    vec3 lookup_dir = vec3(env_dir.x, -env_dir.y, env_dir.z);
    return textureLod(skybox_env, lookup_dir, specular_lod).rgb;
}

vec3 SampleEnvDiffuse(vec3 normal_world, mat3 env_rot_inv, float max_env_lod)
{
    vec3 env_dir = env_rot_inv * normal_world;
    vec3 lookup_dir = vec3(env_dir.x, -env_dir.y, env_dir.z);
    return textureLod(skybox_env, lookup_dir, max_env_lod).rgb;
}

vec3 SampleReflection(
    vec3 origin_model,
    vec3 dir_model,
    vec3 dir_world,
    mat3 env_rot_inv,
    float roughness,
    float max_env_lod)
{
    vec3 env_color =
        SampleEnvSpecular(dir_world, env_rot_inv, roughness, max_env_lod);
    vec3 reflection_sample = env_color;
    HitInfo reflection_hit = TraceScene(origin_model, dir_model);
    if (reflection_hit.hit)
    {
        reflection_sample = texture(albedo_texture, reflection_hit.uv).rgb;
        reflection_sample = mix(env_color, reflection_sample, 0.7);
    }
    return reflection_sample;
}

bool RefractDir(
    vec3 ray_dir,
    vec3 normal_world,
    float ior,
    out vec3 refract_dir,
    out float cosi)
{
    float cosi_local = clamp(dot(ray_dir, normal_world), -1.0, 1.0);
    float etai = 1.0;
    float etat = ior;
    vec3 n = normal_world;
    if (cosi_local > 0.0)
    {
        n = -normal_world;
        float tmp = etai;
        etai = etat;
        etat = tmp;
    }
    cosi = abs(cosi_local);
    float eta = etai / etat;
    refract_dir = refract(ray_dir, n, eta);
    return length(refract_dir) > 0.001;
}

vec3 ShadeOpaque(
    const HitInfo hit,
    vec3 ray_dir_world,
    mat3 model_inv3,
    mat3 normal_matrix,
    vec3 light_dir_world,
    vec3 light_color_world,
    mat3 env_rot_inv,
    float max_env_lod,
    bool allow_reflection)
{
    vec3 N = normalize(normal_matrix * hit.normal_model);
    vec3 T = normalize(normal_matrix * hit.tangent_model);
    vec3 B = normalize(normal_matrix * hit.bitangent_model);
    if (length(T) < 0.001 || length(B) < 0.001)
    {
        vec3 axis = abs(N.z) > 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
        T = normalize(cross(axis, N));
        B = normalize(cross(N, T));
    }
    vec3 normal_map = texture(normal_texture, hit.uv).xyz * 2.0 - 1.0;
    vec3 hit_normal = normalize(mat3(T, B, N) * normal_map);
    if (length(hit_normal) < 0.001)
    {
        hit_normal = N;
    }

    vec3 dir = length(light_dir_world) > 0.0 ? light_dir_world
                                             : vec3(1.0, -1.0, 1.0);
    vec3 col = length(light_color_world) > 0.0 ? light_color_world
                                               : vec3(1.0);

    vec3 shadow_dir = normalize(model_inv3 * -dir);
    vec3 shadow_origin = hit.pos_model + hit.normal_model * 0.0015;
    bool in_shadow = anyHitTriangles(shadow_origin, shadow_dir);
    float shadow_factor = in_shadow ? 0.3 : 1.0;

    vec3 albedo = texture(albedo_texture, hit.uv).rgb;
    float roughness = texture(roughness_texture, hit.uv).r;
    float metallic = texture(metallic_texture, hit.uv).r;
    float ao = texture(ao_texture, hit.uv).r;

    bool is_ground = IsGround(hit);
    if (is_ground)
    {
        albedo = mix(albedo, vec3(0.08), 0.6);
        roughness = min(roughness, 0.25);
        metallic = min(metallic, 0.05);
        ao = 1.0;
    }

    vec3 V = normalize(-ray_dir_world);
    vec3 L = normalize(-dir);
    vec3 H = normalize(V + L);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float D = DistributionGGX(hit_normal, H, roughness);
    float G = GeometrySmith(hit_normal, V, L, roughness);
    vec3 numerator = D * G * F;
    float denominator =
        4.0 * max(dot(hit_normal, V), 0.0) * max(dot(hit_normal, L), 0.0) +
        0.001;
    vec3 specular = numerator / denominator;
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / 3.14159265;
    float NdotL = max(dot(hit_normal, L), 0.0);
    float NdotV = max(dot(hit_normal, V), 0.0);

    vec3 reflection_dir_world = normalize(reflect(-ray_dir_world, hit_normal));
    vec3 reflection_dir_model = normalize(model_inv3 * reflection_dir_world);
    vec3 env_color =
        SampleEnvSpecular(
            reflection_dir_world, env_rot_inv, roughness, max_env_lod);
    vec3 env_diffuse = SampleEnvDiffuse(N, env_rot_inv, max_env_lod);

    vec3 reflection_sample = env_color;
    bool should_trace_reflection = allow_reflection &&
        (is_ground || (NdotV > 0.2 && roughness < 0.65));
    if (should_trace_reflection)
    {
        vec3 reflection_origin_model = hit.pos_model + hit.normal_model * 0.0015;
        reflection_sample = SampleReflection(
            reflection_origin_model,
            reflection_dir_model,
            reflection_dir_world,
            env_rot_inv,
            roughness,
            max_env_lod);
    }

    vec3 ambient = kD * albedo * 0.05 * ao +
                   kD * albedo / 3.14159265 * env_diffuse * ao;
    vec3 env_specular =
        specular * reflection_sample * ao * (in_shadow ? 0.0 : 1.0);
    if (is_ground)
    {
        env_specular *= 1.35;
    }
    env_specular = clamp(env_specular, 0.0, 10.0);
    vec3 Lo = diffuse * col * NdotL * shadow_factor +
              specular * col * NdotL * (in_shadow ? 0.0 : 1.0) +
              env_specular;
    return ambient + Lo;
}

vec3 ShadeGlass(
    const HitInfo hit,
    vec3 ray_dir_world,
    mat3 model_inv3,
    mat3 normal_matrix,
    vec3 light_dir_world,
    vec3 light_color_world,
    mat3 env_rot_inv,
    float max_env_lod)
{
    const int kMaxGlassDepth = 4;
    const float ior = 1.5;
    const float kSurfaceBias = 0.01;
    const float kGlassFresnelBoost = 1.35;
    const float kGlassRimStrength = 0.25;
    const float kGlassRimPower = 3.0;
    const vec3 glass_tint = vec3(0.9, 0.95, 1.0);
    const vec3 absorption = vec3(0.15, 0.07, 0.02);

    vec3 accum = vec3(0.0);
    vec3 throughput = vec3(1.0);
    vec3 dir_world = normalize(ray_dir_world);
    vec3 origin_model = hit.pos_model;
    vec3 dir_model = normalize(model_inv3 * dir_world);
    HitInfo current_hit = hit;
    bool inside = false;
    bool terminated = false;

    for (int depth = 0; depth < kMaxGlassDepth; ++depth)
    {
        if (depth > 0)
        {
            current_hit = TraceScene(origin_model, dir_model);
            if (!current_hit.hit)
            {
                accum += throughput *
                         SampleEnvSpecular(
                             dir_world, env_rot_inv, 0.0, max_env_lod);
                terminated = true;
                break;
            }
        }

        if (inside)
        {
            float travel = current_hit.t;
            throughput *= exp(-absorption * travel);
        }

        if (IsGround(current_hit))
        {
            accum += throughput * ShadeOpaque(
                current_hit,
                dir_world,
                model_inv3,
                normal_matrix,
                light_dir_world,
                light_color_world,
                env_rot_inv,
                max_env_lod,
                true);
            terminated = true;
            break;
        }

        vec3 N = normalize(normal_matrix * current_hit.normal_model);
        vec3 I = normalize(dir_world);
        vec3 refract_dir_world;
        float cosi;
        bool has_refraction =
            RefractDir(I, N, ior, refract_dir_world, cosi);
        float fresnel = fresnelDielectric(cosi, ior);
        fresnel = clamp(fresnel * kGlassFresnelBoost, 0.0, 1.0);
        vec3 V = normalize(-dir_world);
        float rim = inside ? 0.0
                           : pow(1.0 - max(dot(N, V), 0.0), kGlassRimPower);
        float rim_factor = clamp(rim * kGlassRimStrength, 0.0, 1.0);
        float reflect_weight = clamp(fresnel + rim_factor, 0.0, 1.0);

        vec3 reflect_dir_world = normalize(reflect(I, N));
        vec3 reflect_dir_model = normalize(model_inv3 * reflect_dir_world);
        vec3 reflect_origin_model =
            current_hit.pos_model + current_hit.normal_model * 0.0015;
        vec3 reflect_color = SampleReflection(
            reflect_origin_model,
            reflect_dir_model,
            reflect_dir_world,
            env_rot_inv,
            0.02,
            max_env_lod);
        accum += throughput * reflect_weight * reflect_color;

        if (!has_refraction)
        {
            break;
        }

        throughput *= (1.0 - reflect_weight);
        if (!inside)
        {
            throughput *= glass_tint;
        }

        dir_world = normalize(refract_dir_world);
        dir_model = normalize(model_inv3 * dir_world);
        vec3 normal_model = normalize(current_hit.normal_model);
        float side = dot(dir_model, normal_model) > 0.0 ? 1.0 : -1.0;
        origin_model = current_hit.pos_model + normal_model * side * kSurfaceBias;
        inside = !inside;
    }

    if (!terminated)
    {
        accum += throughput *
                 SampleEnvSpecular(dir_world, env_rot_inv, 0.0, max_env_lod);
    }
    return accum;
}

void main()
{
    // Reconstruct the view ray for this fragment.
    vec2 uv = out_uv * 2.0 - 1.0;
    vec4 clip_pos = vec4(uv, -1.0, 1.0);
    vec4 view_pos = projection_inv * clip_pos;
    view_pos = vec4(view_pos.xy, -1.0, 0.0);
    vec3 ray_dir_world = normalize((view_inv * view_pos).xyz);

    // Transform the ray to model space to match the triangle buffer.
    mat4 inv_model4 = model_inv;
    mat3 model_inv3 = mat3(inv_model4);
    float inv_det = abs(determinant(model_inv3));
    if (inv_det < 1e-8)
    {
        inv_model4 = inverse(model);
        model_inv3 = mat3(inv_model4);
    }
    mat3 normal_matrix = transpose(model_inv3);
    vec3 ray_origin = (inv_model4 * vec4(camera_position, 1.0)).xyz;
    vec3 ray_dir = normalize(model_inv3 * ray_dir_world);

    HitInfo hit = TraceScene(ray_origin, ray_dir);
    int env_levels = textureQueryLevels(skybox_env);
    float max_env_lod = env_levels > 0 ? float(env_levels - 1) : 0.0;
    mat3 env_rot = mat3(env_map_model);
    float env_det = abs(determinant(env_rot));
    if (env_det < 1e-6)
    {
        env_rot = mat3(1.0);
    }
    mat3 env_rot_inv = transpose(env_rot);
    if (!hit.hit)
    {
        vec3 env_dir = env_rot_inv * ray_dir_world;
        env_dir = vec3(env_dir.x, -env_dir.y, env_dir.z);
        vec3 env_color = textureLod(skybox, env_dir, 0.0).rgb;
        frag_color = vec4(env_color, 1.0);
        return;
    }

    vec3 color;
    if (IsGround(hit))
    {
        color = ShadeOpaque(
            hit,
            ray_dir_world,
            model_inv3,
            normal_matrix,
            light_dir,
            light_color,
            env_rot_inv,
            max_env_lod,
            true);
    }
    else
    {
        color = ShadeGlass(
            hit,
            ray_dir_world,
            model_inv3,
            normal_matrix,
            light_dir,
            light_color,
            env_rot_inv,
            max_env_lod);
    }

    frag_color = vec4(color, 1.0);
}
