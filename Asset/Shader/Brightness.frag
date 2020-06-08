#version 330 core

in vec2 vert_texcoord;

layout (location = 0) out vec4 frag_color;

uniform sampler2D Display;

void main()
{
    vec4 lighting = texture(Display, vert_texcoord);
    
    // check whether fragment output is higher than threshold, if so output as
    // brightness color
    float brightness = dot(lighting.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        frag_color = vec4(lighting.rgb, 1.0);
    else
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
}