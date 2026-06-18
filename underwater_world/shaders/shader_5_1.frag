#version 430 core

in vec3 worldNormal;
in vec3 worldPos;

uniform vec3 color;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 cameraPos;

out vec4 outColor;

void main()
{
    vec3 normal = normalize(worldNormal);
    vec3 light  = normalize(lightDir);
    vec3 view   = normalize(cameraPos - worldPos);
    vec3 refl   = reflect(-light, normal);

    float diff = max(dot(normal, light), 0.0);
    float spec = pow(max(dot(view, refl), 0.0), 32.0);

    vec3 ambient  = 0.3 * lightColor;
    vec3 diffuse  = diff * lightColor;
    vec3 specular = 0.4 * spec * lightColor;

    float dist      = length(worldPos - cameraPos);
    float fogFactor = clamp(exp(-dist * 0.04), 0.0, 1.0);
    vec3  fogColor  = vec3(0.0, 0.15, 0.25);

    vec3 litColor = (ambient + diffuse + specular) * color;
    outColor = vec4(mix(fogColor, litColor, fogFactor), 1.0);
}
