#version 330 core

in vec3 vert_world_position;
in vec3 vert_normal;
in vec2 vert_texcoord;

layout (location = 0) out vec4 frag_color;

uniform sampler2D Color;
uniform sampler2D Normal;
uniform sampler2D Metallic;
uniform sampler2D Roughness;
uniform sampler2D AmbientOcclusion;

uniform samplerCube MonteCarloPrefilter;
uniform samplerCube Irradiance;
uniform sampler2D IntegrateBRDF;

uniform vec3 camera_position;

uniform vec3 light_position[4];
uniform vec3 light_color[4];

const float PI = 3.14159265359;

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
    vec3 T   = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
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

    // reflectance equation
    vec3 Lo = vec3(0.0);
    
#if 1
    for (int i = 0; i < 4; ++i)
    {
        // calculate per-light radiance
        vec3 L = normalize(light_position[i] - vert_world_position);
        vec3 H = normalize(V + L);
        float distance = length(light_position[i] - vert_world_position);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = light_color[i] * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 nominator = NDF * G * F; 
        // 0.001 to prevent divide by zero.
        float denominator = 
            4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = nominator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        // note that we already multiplied the BRDF by the Fresnel (kS) so we
        // won't multiply by kS again
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
#endif

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
    
    vec3 color = ambient + Lo;

    // deactivate the HDR as this is computed in the Bloom filter
#if 0
    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 
#endif

    frag_color = vec4(color, 1.0);
}
