#version 450

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform texture2D default_image;
layout(set = 1, binding = 1) uniform sampler default_sampler;

layout (std140, set = 1, binding = 2) uniform TilemapProps
{
    float width_scale;
    float height_scale;
} tilemap_props;


void main()
{
    out_color = texture(sampler2D(default_image, default_sampler), in_uv.st);
    out_color = texture(sampler2D(default_image, default_sampler), in_uv.st * vec2(tilemap_props.width_scale, tilemap_props.height_scale));
    // out_color = vec4(0.2, in_uv.x, in_uv.y, 1.0);
}
