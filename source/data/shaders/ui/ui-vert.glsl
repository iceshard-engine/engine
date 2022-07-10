#version 450

layout (location = 0) in vec2 in_pos;
// layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec4 in_color;
layout(push_constant) uniform push_const { vec2 scale; vec2 translate; } pc;

// layout (location = 0) out vec2 out_uv;
layout (location = 1) out vec4 out_color;

layout (std140, set = 0, binding = 0) uniform UI
{
    vec2 position;
    vec2 scale;
} ui;

void main()
{
    // out_uv = in_uv;
    out_color = in_color;
    gl_Position = vec4(in_pos * pc.scale + pc.translate, 0, 1);
    // gl_Position = vec4(in_pos * pc.scale * ui.scale + pc.translate + ui.position * pc.scale, 0, 1);
}
