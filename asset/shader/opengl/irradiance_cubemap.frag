#version 410 core

in vec3 vert_local;

layout(location = 0) out vec4 frag_color;

uniform samplerCube Environment;

const float PI = 3.14159265359;

void main()
{		
    // The world vector acts as the normal of a tangent surface
    // from the origin, aligned to out_local. Given this normal, calculate all
    // incoming radiance of the environment. The result of this radiance
    // is the radiance of light coming from -Normal direction, which is what
    // we use in the PBR shader to sample irradiance.
    // Invert X and Y so irradiance map matches skybox orientation.
    vec3 normal = normalize(vec3(-vert_local.x, -vert_local.y, vert_local.z));
    vec3 irradiance = vec3(0.0);

    // Build an orthonormal basis around the normal. Using a fixed up vector
    // causes issues when the normal is parallel to it, which resulted in
    // several faces of the irradiance cube map being black. Choose a new up
    // vector when necessary and normalize the basis vectors so sampling works
    // for all directions.
    vec3 up = abs(normal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 right = normalize(cross(up, normal));
    up = cross(normal, right);

    // Increase the number of samples
    float sampleDensity = 512.0;
    float sampleDelta = 2.0 * PI / sampleDensity;
    float nrSamples = 0.0;

    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

            irradiance += texture(Environment, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    frag_color = vec4(irradiance, 1.0);
}
