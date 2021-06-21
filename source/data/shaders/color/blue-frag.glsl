#version 450

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(0.2, in_uv.x, in_uv.y, 1.0);
}
