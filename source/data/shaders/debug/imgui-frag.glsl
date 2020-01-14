#version 450

layout (location = 0) in vec2 in_uv;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform sampler default_sampler;
layout(set = 0, binding = 2) uniform texture2D default_image;

void main()
{
   out_color = vec4(1, 0,0,1); // in_color * texture(sampler2D(default_image, default_sampler), in_uv);
}
