#version 450

layout (location = 0) in vec2 in_uv;
layout (location = 1) flat in int in_mat;

layout (location = 0) out vec4 out_color;

layout (std140, set = 1, binding = 0) uniform TilemapProps
{
    vec2 scale[4];
} tilemap_props;

layout(set = 1, binding = 1) uniform texture2D default_image[1];

layout(set = 2, binding = 0) uniform sampler default_sampler;

void main()
{
    vec4 tex_color = texture(sampler2D(default_image[in_mat], default_sampler), in_uv.st * tilemap_props.scale[in_mat]);
    out_color = tex_color.rgba;
}
