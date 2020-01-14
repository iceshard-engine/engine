#version 450

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec4 in_color;
layout(push_constant) uniform push_const { vec2 scale; vec2 translate; } pc;

layout (location = 0) out vec2 out_uv;
layout (location = 1) out vec4 out_color;

void main()
{
    out_uv = in_uv;
    out_color = in_color;
    gl_Position = vec4(in_pos * pc.scale + pc.translate, 0, 1);
}
