#version 450

layout (location = 0) in vec4 in_color;
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

layout(std140, binding = 5) uniform Lights
{
    // DirectionalLight dir_light;
    PointLight in_point_light[20];
    // SpotLight spot_light[20];
    int in_point_light_num;
    // int spot_light_num;
};

// vec3 calc_point_light(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_dir)
// {
//     vec3 light_dir = normalize(light.position - frag_pos);

//     float diff = max(dot(normal, light_dir), 0.0);

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
// }

void main()
{
    float ambient_strength = 0.1;
    vec3 ambient = ambient_strength * vec3(0.8, 0.8, 0.8);

    vec3 norm = normalize(in_norm);
    vec3 light_dir = normalize(in_point_light[0].position - in_pos);

    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * vec3(0.8);

    vec3 result = (ambient + diffuse);
    out_color = vec4(result, 1.0f) * in_color;
}
