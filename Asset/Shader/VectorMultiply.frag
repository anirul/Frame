#version 330 core
  
in vec2 vert_texcoord;

layout (location = 0) out vec4 frag_color;

uniform sampler2D Texture0;
uniform sampler2D Texture1;
uniform sampler2D Texture2;
uniform sampler2D Texture3;
uniform sampler2D Texture4;
uniform sampler2D Texture5;
uniform sampler2D Texture6;
uniform sampler2D Texture7;
uniform sampler2D Texture8;
uniform sampler2D Texture9;
uniform sampler2D Texture10;
uniform sampler2D Texture11;
uniform sampler2D Texture12;
uniform sampler2D Texture13;
uniform sampler2D Texture14;
uniform sampler2D Texture15;

uniform int texture_max;

void main()
{
    vec3 total = vec3(1.0, 1.0, 1.0);
    if (texture_max > 0)
        total *= texture(Texture0, vert_texcoord).rgb;
    if (texture_max > 1)
        total *= texture(Texture1, vert_texcoord).rgb;
    if (texture_max > 2)
        total *= texture(Texture2, vert_texcoord).rgb;
    if (texture_max > 3)
        total *= texture(Texture3, vert_texcoord).rgb;
    if (texture_max > 4)
        total *= texture(Texture4, vert_texcoord).rgb; 
    if (texture_max > 5)
        total *= texture(Texture5, vert_texcoord).rgb;
    if (texture_max > 6)
        total *= texture(Texture6, vert_texcoord).rgb;
    if (texture_max > 7)
        total *= texture(Texture7, vert_texcoord).rgb;
    if (texture_max > 8)
        total *= texture(Texture8, vert_texcoord).rgb;
    if (texture_max > 9)
        total *= texture(Texture9, vert_texcoord).rgb;
    if (texture_max > 10)
        total *= texture(Texture10, vert_texcoord).rgb;
    if (texture_max > 11)
        total *= texture(Texture11, vert_texcoord).rgb;
    if (texture_max > 12)
        total *= texture(Texture12, vert_texcoord).rgb;
    if (texture_max > 13)
        total *= texture(Texture13, vert_texcoord).rgb;
    if (texture_max > 14)
        total *= texture(Texture14, vert_texcoord).rgb;
    if (texture_max > 15)
        total *= texture(Texture15, vert_texcoord).rgb;
    frag_color = vec4(total.rgb, 1.0);
}  