#version 450

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec2 in_texCoord;
layout(location = 2) in float in_textureIndex;

layout(location = 0) out vec4 frag_color; 

layout(binding = 0) uniform sampler2D u_textures[32];

void main()
{
    int index = int(in_textureIndex);
    // frag_color = texture(u_textures[index], in_texCoord) * in_color;
    frag_color = in_color;
}