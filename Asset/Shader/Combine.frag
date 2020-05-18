#version 330 core

out vec4 frag_color;
  
in vec2 vert_texcoord;

uniform sampler2D Display;
uniform sampler2D GaussianBlur;

uniform float exposure;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(Display, vert_texcoord).rgb;      
    vec3 bloomColor = texture(GaussianBlur, vert_texcoord).rgb;
    // additive blending
    hdrColor += bloomColor;
    
#if 1
    // tone mapping
    // vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    vec3 result = hdrColor / hdrColor - vec3(1.0);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    frag_color = vec4(result, 1.0);
#else
    frag_color = vec4(hdrColor.rgb, 1.0);
#endif
}  