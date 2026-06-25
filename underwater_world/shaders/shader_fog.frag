#version 430 core

in vec2 texCoord;
in vec3 worldPos;
in vec3 worldNormal;

uniform sampler2D objectTexture; 
uniform vec3 cameraPos;
uniform vec3 fogColor;
uniform float fogDensity;
uniform bool spotOn;
uniform vec3 spotPos;
uniform vec3 spotDir;
uniform float spotCutoff;
uniform vec3 spotColor;

const vec3 lightDir = normalize(vec3(-0.2, 1.0, -0.5));
const vec3 lightColor = vec3(1.0, 1.0, 1.0);

out vec4 outColor;

void main()
{
    vec4 texColor = texture(objectTexture, texCoord);

    vec3 normal = normalize(worldNormal + vec3(0.00001));
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 ambient = 0.4 * lightColor;
    vec3 diffuse = diff * lightColor;
    vec3 litColor = (ambient + diffuse) * texColor.rgb;

    float dist = length(worldPos - cameraPos); 

    vec3 absorption = vec3(0.15, 0.05, 0.01); 
    litColor *= exp(-dist * absorption); 
    if (spotOn) {
        vec3 lightToFrag = normalize(spotPos - worldPos);
        float theta = dot(lightToFrag, normalize(-spotDir)); 
        
        if (theta > spotCutoff) {
            float intensity = smoothstep(spotCutoff, spotCutoff + 0.05, theta);
            float distance = length(spotPos - worldPos);
            float attenuation = 1.0 / (1.0 + 0.04 * distance + 0.01 * (distance * distance));

            vec3 spotDiffuse = max(dot(normal, lightToFrag), 0.0) * spotColor;
            
            litColor += texColor.rgb * spotDiffuse * intensity * attenuation * 3.0; 
        }
    }


    float fogFactor = clamp(exp(-dist * fogDensity), 0.0, 1.0);
    vec3 fogColor = vec3(0.0, 0.15, 0.2);

    outColor = vec4(mix(fogColor, litColor, fogFactor), 1.0);
}