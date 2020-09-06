#version 450

layout (location = 0) in vec2 in_uv;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform sampler default_sampler;
layout(set = 3, binding = 2) uniform texture2D default_image;

void main()
{
    out_color = in_color * texture(sampler2D(default_image, default_sampler), in_uv.st);
}
