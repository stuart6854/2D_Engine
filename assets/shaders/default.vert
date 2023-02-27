#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_texCoord;
layout(location = 3) in float in_textureIndex;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_texCoord;
layout(location = 2) out float out_textureIndex;

layout(push_constant) uniform PushBlock
{
    mat4 viewProj;
    mat4 transform;
} u_consts;

void main()
{
    gl_Position = u_consts.viewProj * u_consts.transform * vec4(in_position, 1.0);

    out_color = in_color;
    out_texCoord = in_texCoord;
    out_textureIndex = in_textureIndex;
}