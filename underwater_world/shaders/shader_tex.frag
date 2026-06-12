#version 430 core

in vec2 texCoord;
in vec3 worldNormal;

uniform sampler2D colorTexture;
uniform vec3 lightDir;
uniform vec3 lightColor;

out vec4 outColor;

void main()
{
    vec4 texColor = texture(colorTexture, texCoord);
    float diff = max(dot(normalize(worldNormal), normalize(lightDir)), 0.0);
    vec3 ambient = 0.3 * lightColor;
    vec3 diffuse = diff * lightColor;
    outColor = vec4((ambient + diffuse) * texColor.rgb, texColor.a);
}