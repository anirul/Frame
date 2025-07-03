#version 410 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

out vec4 fragColor;

uniform float ibl_exposure = 0.75;
uniform sampler2D Color;
uniform samplerCube IrradianceMap;

void main()
{
    // Sample the diffuse texture.
    vec3 diffuseColor = texture(Color, fragTexCoord).rgb;
    // Sample the irradiance map using the transformed normal.
    vec3 normalizeDirection =  normalize(vec3(fragNormal.x, -fragNormal.y, fragNormal.z));
    vec3 irradiance = texture(IrradianceMap, normalizeDirection).rgb;
    // Mix the irradiance with the diffuse color.
    vec3 mixedColor = mix(irradiance, diffuseColor.rgb, ibl_exposure);
    // Output the mixed color.
    fragColor = vec4(mixedColor, 1.0);
}
