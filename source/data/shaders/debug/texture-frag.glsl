#version 450

layout (location = 0) in vec2 in_uv;
layout (location = 1) in vec3 in_pos;
layout (location = 2) in vec3 in_norm;

layout (location = 0) out vec4 out_color;

struct PointLight
{
    vec3 position;
    // vec3 ambient;
    // vec3 diffuse;
    // vec3 specular;

    // float constant;
    // float linear;
    // float quadratic;
};

layout(std140, set = 1, binding = 5) uniform Lights
{
    // DirectionalLight dir_light;
    PointLight in_point_light[20];
    // SpotLight spot_light[20];
    int in_point_light_num;
    // int spot_light_num;
};

vec3 calc_point_light(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_dir)
{
    vec3 light_dir = normalize(light.position.xyz - frag_pos);

    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diff * vec3(0.8);

//     vec3 reflect_dir = reflect(-light_dir, normal);
//     float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material_shininess);

//     // Attenuation
//     float distance = length(light.position - frag_pos);
//     float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

//     // Combine
//     vec3 ambient = light.ambient * material_diffuse;
//     vec3 diffuse = light.diffuse * diff * material_diffuse;
//     vec3 specular = light.specular * spec * material_specular;
//     ambient *= attenuation;
//     diffuse *= attenuation;
//     specular *= attenuation;
//     return (ambient + diffuse + specular);
    return diffuse;
}

layout(set = 2, binding = 0) uniform sampler default_sampler;

layout(set = 3, binding = 2) uniform texture2D normal_image;
layout(set = 3, binding = 3) uniform texture2D diffuse_image;
layout(set = 3, binding = 4) uniform texture2D specular_image;

void main()
{
    float ambient_strength = 0.1;
    vec3 ambient = ambient_strength * vec3(0.8, 0.8, 0.8);

    vec3 norm = normalize(in_norm);

    vec3 temp_color = vec3(0.0);
    for(int i = 0; i < in_point_light_num; i++)
    {
        temp_color += calc_point_light(in_point_light[i], norm, in_pos, vec3(0.0));
    }

    vec4 color = texture(sampler2D(diffuse_image, default_sampler), in_uv.st);

    temp_color += ambient;
    out_color = vec4(temp_color, 1.0f) * color;
}
