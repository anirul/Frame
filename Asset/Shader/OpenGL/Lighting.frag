#version 330 core

in vec2 vert_texcoord;

layout (location = 0) out vec4 frag_color;

uniform sampler2D Ambient;
uniform sampler2D Normal;
uniform sampler2D MetalRoughAO;
uniform sampler2D Position;

uniform vec3 camera_position;
uniform vec3 light_position[32];
uniform vec3 light_color[32];
uniform int light_max;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
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

void main()
{
    // Surface come from textures.
    vec3 albedo = texture(Ambient, vert_texcoord).rgb;
    vec3 MRA = texture(MetalRoughAO, vert_texcoord).rgb;
    float metallic = MRA.x;
    float roughness = MRA.y;
    vec3 N = texture(Normal, vert_texcoord).xyz;
    vec3 world_position = texture(Position, vert_texcoord).xyz;

    // In cas they are both 0 then this is the background.
    if (MRA == vec3(0, 0, 0) && world_position == vec3(0, 0, 0))
    {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    vec3 V = normalize(camera_position - world_position);

    // calculate reflectance at normal incidence; if dia-electric (like plastic)
    // use F0 of 0.04 and if it's a metal, use the albedo color as F0 (metallic
    // workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    
    for (int i = 0; i < light_max; ++i)
    {
        // calculate per-light radiance
        vec3 L = normalize(light_position[i] - world_position);
        vec3 H = normalize(V + L);
        float dist = length(light_position[i] - world_position);
        float attenuation = 1.0 / (dist * dist);
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
        Lo += max((kD * albedo / PI + specular) * radiance * NdotL, 0.0);
    }

	frag_color = vec4(Lo, 1.0);
}