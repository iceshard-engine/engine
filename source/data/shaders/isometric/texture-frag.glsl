#version 450

layout (location = 0) in vec2 in_uv;
layout (location = 1) in vec3 in_pos;
layout (location = 2) in vec3 in_norm;

layout (location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform sampler default_sampler;

layout(set = 3, binding = 3) uniform texture2D diffuse_image;

void main()
{
    float ambient_strength = 0.8;
    vec3 ambient = ambient_strength * vec3(0.8, 0.8, 0.8);

    vec4 color = texture(sampler2D(diffuse_image, default_sampler), in_uv.st);

    out_color = vec4(ambient, 1.0f) * color;
}
