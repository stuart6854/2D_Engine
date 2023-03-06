#version 450

layout(location = 0) in vec2 in_texCoord;

layout(location = 0) out vec4 frag_color; 

layout(binding = 0) uniform sampler2D u_texture;

void main()
{
    // frag_color = vec4(in_texCoord, 0.0, 1.0);
    // int index = int(in_textureIndex);
    frag_color = texture(u_texture, in_texCoord);
}