#version 330 core

out vec4 frag_color;
  
in vec2 vert_texcoord;

uniform sampler2D Image;
  
uniform bool horizontal;

uniform float weight[5] =
    float[] (0.382928, 0.241732, 0.060598, 0.005977, 0.000229);
//  float[] (0.146634, 0.092566, 0.023205, 0.002289, 0.000088);
//  float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{             
    // gets size of single texel
    vec2 tex_offset = 1.0 / textureSize(Image, 0); 
    // current fragment's contribution
    vec3 result = texture(Image, vert_texcoord).rgb * weight[0]; 
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(
                Image, 
                vert_texcoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(
                Image, 
                vert_texcoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(
                Image, 
                vert_texcoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(
                Image, 
                vert_texcoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    frag_color = vec4(result, 1.0);
}
