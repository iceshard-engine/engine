#version 450

layout (location = 0) in vec2 in_uv;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform texture2D default_image;
layout(set = 0, binding = 1) uniform sampler default_sampler;

void main()
{
    out_color = in_color * texture(sampler2D(default_image, default_sampler), in_uv.st);
}
