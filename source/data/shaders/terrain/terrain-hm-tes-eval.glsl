#version 450

layout( quads, equal_spacing, cw ) in;
layout( location = 0 ) in vec2 in_uv[];
layout( location = 1 ) in vec2 vertexTexCoord[];

layout(set = 1, binding = 0) uniform texture2D hm_image;
layout(set = 1, binding = 1) uniform sampler hm_sampler;

layout(location = 0) out float tes_height;

void main()
{
//    vec4 pos =
//        gl_in[0].gl_Position * gl_TessCoord.x +
//        gl_in[1].gl_Position * gl_TessCoord.y +
//        gl_in[2].gl_Position * gl_TessCoord.z;

	vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 pos = mix(pos1, pos2, gl_TessCoord.y);

//    vec2 uv =
//        in_uv[0] * gl_TessCoord.x +
//        in_uv[1] * gl_TessCoord.y +
//        in_uv[2] * gl_TessCoord.z;

	vec2 uv1 = mix(in_uv[0], in_uv[1], gl_TessCoord.x);
	vec2 uv2 = mix(in_uv[3], in_uv[2], gl_TessCoord.x);
	vec2 uv = gl_TessCoord.xy;// mix(uv1, uv2, gl_TessCoord.y);

    float height = texture(sampler2D(hm_image, hm_sampler), uv).x;
    pos.y += height * 2;
    gl_Position = pos;
    tes_height = height;
}
