#version 450

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform sampler default_sampler;

layout(input_attachment_index = 0, set = 0, binding = 2) uniform subpassInput rendered_image;

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
    vec2 resolution = vec2(-1.0, 1.0);

    vec2 uuv = in_uv.st / resolution.xy;
    uuv *= 1.0 - uuv.yx;
    float vig = uuv.x * uuv.y * 15;
    vig = pow(vig, 0.25);


    // vec4 col = texture(sampler2D(rendered_image, default_sampler), in_uv.st);
    vec4 col = subpassLoad(rendered_image);
    // float stl = clamp(length(in_uv.st - vec2(0.5, 0.5)), 0.0,1.0);
    // float lum = col.r * 0.7 + col.g * 0.2 + col.b * 0.1;
    out_color = vec4(col.rgb, 1.0); //sepia(col.rgb); // vig * pow(1-length((in_uv.st - vec2(0.5)) / 2) * 5, 1.25); // * vec3(vig);
}
