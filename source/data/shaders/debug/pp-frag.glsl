#version 450

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec3 out_color;

layout(set = 1, binding = 0) uniform sampler default_sampler;
layout(set = 2, binding = 2) uniform texture2D rendered_image;

vec3 sepia(vec3 color)
{
    return vec3(
        (color.r * 0.393) + (color.g * 0.769) + (color.b * 0.189),
        (color.r * 0.349) + (color.g * 0.686) + (color.b * 0.168),
        (color.r * 0.272) + (color.g * 0.534) + (color.b * 0.131)
    );
}

void main()
{
    vec4 col = texture(sampler2D(rendered_image, default_sampler), in_uv.st);
    // float stl = clamp(length(in_uv.st - vec2(0.5, 0.5)), 0.0,1.0);
    // float lum = col.r * 0.7 + col.g * 0.2 + col.b * 0.1;
    out_color = sepia(col.rgb);
}
