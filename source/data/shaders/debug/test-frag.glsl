#version 450

layout (location = 0) in vec4 color;
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler default_sampler;
layout(set = 0, binding = 2) uniform texture2D default_image;

void main()
{
   outColor = color; // texture(sampler2D(default_image, default_sampler), vec2(0.2, 0.3));
}
