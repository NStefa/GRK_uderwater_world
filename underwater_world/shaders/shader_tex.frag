#version 430 core

in vec2 texCoord;
in vec3 worldNormal;
in vec3 worldPos;

uniform sampler2D colorTexture;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 cameraPos;

uniform bool spotOn;
uniform vec3 spotPos;
uniform vec3 spotDir;
uniform float spotCutoff;
uniform vec3 spotColor;

out vec4 outColor;

void main()
{
    vec4 texColor = texture(colorTexture, texCoord);
    vec3 normal = normalize(worldNormal);
    float diff = max(dot(normal, normalize(lightDir)), 0.0);
    vec3 ambient = 0.2 * lightColor;
    vec3 diffuse = diff * lightColor;
    vec3 color = (ambient + diffuse) * texColor.rgb;

    if (spotOn) {
        vec3 lightToFrag = normalize(spotPos - worldPos);
        float theta = dot(lightToFrag, normalize(-spotDir));
        if (theta > spotCutoff) {
            float intensity = smoothstep(spotCutoff, spotCutoff + 0.05, theta);
            float distance = length(spotPos - worldPos);
            float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
            vec3 spotDiffuse = max(dot(normal, lightToFrag), 0.0) * spotColor;
            color += spotDiffuse * intensity * attenuation * texColor.rgb;
        }
    }

    float dist = length(worldPos - cameraPos);
    float fogFactor = clamp(exp(-dist * 0.04), 0.0, 1.0);
    vec3 fogColor = vec3(0.0, 0.15, 0.25);

    outColor = vec4(mix(fogColor, color, fogFactor), texColor.a);
}