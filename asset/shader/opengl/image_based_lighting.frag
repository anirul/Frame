#version 450 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

out vec4 fragColor;

uniform sampler2D Color;
uniform samplerCube IrradianceMap;

void main()
{
    // Sample the diffuse texture
    vec3 diffuseColor = texture(Color, fragTexCoord).rgb;

    // Sample the irradiance map why do I have to invert the normal?
    vec3 irradiance = texture(IrradianceMap, -fragNormal).rgb;

    // Multiply the diffuse color with the irradiance color
    vec3 mixedColor = mix(irradiance, diffuseColor.rgb, 0.75);

    // Output the mixed color
    fragColor = vec4(mixedColor, 1.0);
}
