#version 450

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(0.2, 0.2, 0.7, 1.0);
}
