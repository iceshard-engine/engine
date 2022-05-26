#version 450
#extension GL_EXT_samplerless_texture_functions : enable

// layout (location = 0) in vec4 in_color;
layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler default_sampler;
layout(set = 1, binding = 0) uniform texture2D default_image;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float screenPxRange() {
    vec2 unitRange = vec2(2.0)/vec2(textureSize(default_image, 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(in_uv);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

void main()
{
    vec4 in_bg_color = vec4(0.0);
    vec4 in_fg_color = vec4(1.0); //vec4(0.0, 0.0, 0.0, 1.0);

    vec4 msdf = texture(sampler2D(default_image, default_sampler), in_uv.st).rgba;
    float sd = median(msdf.r, msdf.g, msdf.b);
    float screen_px_distance = screenPxRange() * (sd - 0.5);
    float opacity = clamp(screen_px_distance + 0.5, 0.0, 1.0);

    out_color = mix(in_bg_color, in_fg_color, opacity);
}
