#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texCoord;

layout(location = 0) out vec2 out_texCoord;

layout(push_constant) uniform PushBlock
{
    mat4 viewProj;
    mat4 transform;
} u_consts;

void main()
{
    gl_Position = u_consts.viewProj * u_consts.transform * vec4(in_position, 0.0, 1.0);

    out_texCoord = in_texCoord;
}