#version 430 core

in vec2 texCoord;
in vec3 worldNormal;
in vec3 worldPos;
in vec3 worldTangent;
in vec3 worldBitangent;
in vec4 fragPosLightSpace;

uniform sampler2D shadowMap;
uniform sampler2D flowMap;
uniform sampler2D colorTexture;
uniform sampler2D normalMap;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 cameraPos;

uniform float time;
uniform float speed;
uniform float flowScale;
uniform float flowMapScale;
uniform vec2 flowDirection;
uniform bool flowColor;

uniform bool spotOn;
uniform vec3 spotPos;
uniform vec3 spotDir;
uniform float spotCutoff;
uniform vec3 spotColor;

out vec4 outColor;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0)
        return 0.0;

    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    float currentDepth = projCoords.z;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    return shadow / 9.0; 
}

void main()
{
    vec2 flowUV = texCoord * flowMapScale;
    vec2 flowRaw = texture(flowMap, flowUV).rg * 2.0 - 1.0;
    vec2 fDir = flowRaw * 0.3 + flowDirection * 0.7;

    vec2 flow = length(fDir) > 0.001 ? normalize(fDir) : vec2(0.0);

    float phase0 = fract(time * speed);
    float phase1 = fract(time * speed + 0.5);

    vec2 uv0 = texCoord - flow * phase0 * flowScale;
    vec2 uv1 = texCoord - flow * phase1 * flowScale;

    float blend = abs(phase0 * 2.0 - 1.0);

    vec4 texColor;
    if (flowColor) {
        vec4 c0 = texture(colorTexture, uv0);
        vec4 c1 = texture(colorTexture, uv1);
        texColor = mix(c0, c1, blend);
    } else {
        texColor = texture(colorTexture, texCoord);
    }

    vec3 n0 = texture(normalMap, uv0).rgb * 2.0 - 1.0;
    vec3 n1 = texture(normalMap, uv1).rgb * 2.0 - 1.0;
    vec3 tangentNormal = normalize(mix(n0, n1, blend));

    // sprawdzanie d�ugo�ci wektor�w i normalizacja, aby unikn�� problem�w z zerow� d�ugo�ci�
    vec3 T = length(worldTangent) > 0.001 ? normalize(worldTangent) : vec3(1.0, 0.0, 0.0);
    vec3 B = length(worldBitangent) > 0.001 ? normalize(worldBitangent) : vec3(0.0, 1.0, 0.0);
    vec3 N = length(worldNormal) > 0.001 ? normalize(worldNormal) : vec3(0.0, 0.0, 1.0);

    mat3 TBN = mat3(T, B, N);
    vec3 normal = normalize(TBN * tangentNormal);

    float diff = max(dot(normal, normalize(lightDir)), 0.0);
    float shadow = ShadowCalculation(fragPosLightSpace, normal, normalize(lightDir));
    
    vec3 ambient = 0.3 * lightColor;
    vec3 diffuse = diff * lightColor * (1.0 - shadow);
    vec3 litColor = (ambient + diffuse) * texColor.rgb;

    // latarka
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

    float dist = length(worldPos - cameraPos);
    float fogFactor = clamp(exp(-dist * 0.04), 0.0, 1.0);
    vec3 fogColor = vec3(0.0, 0.15, 0.25);

    outColor = vec4(mix(fogColor, litColor, fogFactor), texColor.a);
}