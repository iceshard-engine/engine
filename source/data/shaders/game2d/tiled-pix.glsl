#version 450

layout (location = 0) in vec2 in_uv;
layout (location = 1) flat in uint in_mat;

layout (location = 0) out vec4 out_color;

layout(set = 0, binding = 2) uniform sampler default_sampler;
layout(set = 1, binding = 3) uniform texture2D default_image[4];

layout (std140, set = 1, binding = 4) uniform TilemapProps
{
    vec2 tile_scale;
    vec2 tile_size;
} tilemap_props[4];

void main()
{
    vec4 tex_color = texture(sampler2D(default_image[in_mat], default_sampler), in_uv.st * tilemap_props[in_mat].tile_scale);
    out_color = tex_color.rgba;
}
