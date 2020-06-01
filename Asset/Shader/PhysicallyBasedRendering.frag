#version 330 core

in vec3 vert_world_position;
in vec3 vert_normal;
in vec2 vert_texcoord;

layout (location = 0) out vec4 frag_ambient;
layout (location = 1) out vec4 frag_normal;
layout (location = 2) out vec4 frag_mra;
layout (location = 3) out vec4 frag_position;

uniform sampler2D Color;
uniform sampler2D Normal;
uniform sampler2D Metallic;
uniform sampler2D Roughness;
uniform sampler2D AmbientOcclusion;

uniform samplerCube MonteCarloPrefilter;
uniform samplerCube Irradiance;
uniform sampler2D IntegrateBRDF;

uniform vec3 camera_position;

// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
vec3 getNormalFromMap(vec3 normalMapPosition)
{
    vec3 tangentNormal = normalMapPosition * 2.0 - 1.0;

    vec3 Q1  = dFdx(vert_world_position);
    vec3 Q2  = dFdy(vert_world_position);
    vec2 st1 = dFdx(vert_texcoord);
    vec2 st2 = dFdy(vert_texcoord);

    vec3 N   = normalize(vert_normal);
    vec3 T   = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return 
        F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   

// ----------------------------------------------------------------------------
void main()
{
#if 1
    // Surface come from textures.
    vec3 albedo       = pow(texture(Color, vert_texcoord).rgb, vec3(2.2));
    float metallic    = texture(Metallic, vert_texcoord).r;
    float roughness   = texture(Roughness, vert_texcoord).r;
    float ao          = texture(AmbientOcclusion, vert_texcoord).r;
    vec3 normal       = texture(Normal, vert_texcoord).xyz;
    vec3 N            = getNormalFromMap(normal);
    vec3 irradiance   = texture(Irradiance, N).rgb;
#else
    // Red metallic surface.
    vec3 albedo       = pow(vec3(0.5, 0.0, 0.0), vec3(2.2));
    float metallic    = 1.0;
    float roughness   = 0.2; // This has to be > 0.
    float ao          = 1.0;
    vec3 normal       = vec3(0.5, 0.5, 1.0);
    vec3 N            = getNormalFromMap(normal);
    vec3 irradiance   = vec3(0.0, 1.0, 0.0);
#endif
    
    vec3 V = normalize(camera_position - vert_world_position);
    vec3 R = reflect(-V, N);

    // calculate reflectance at normal incidence; if dia-electric (like plastic)
    // use F0 of 0.04 and if it's a metal, use the albedo color as F0 (metallic
    // workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 diffuse = irradiance * albedo;

    // sample both the pre-filter map and the BRDF lut and combine them together
    // as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = 
        textureLod(MonteCarloPrefilter, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(IntegrateBRDF, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;

    frag_ambient = vec4(ambient, 1.0);
    frag_normal = vec4(N, 1.0);
    frag_mra = vec4(metallic, roughness, ao, 1.0);
    frag_position = vec4(vert_world_position, 1.0);
}
