#version 430 core

const float PI = 3.14159265359;

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;
in mat3 TBN;
in vec4 fragPosLightSpace;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 camPos;

// latarka
uniform bool spotOn;
uniform vec3 spotPos;
uniform vec3 spotDir;
uniform float spotCutoff;
uniform vec3 spotColor;
uniform sampler2D shadowMap;
uniform bool flipNormals;

out vec4 outColor;

// rozklad normalnych GGX
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// funkcja geometrii Schlick-GGX
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// funkcja geometrii Smitha
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

// obliczanie cieni z PCF
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    float currentDepth = projCoords.z;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

// aproksymacja Fresnela (Schlick)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    // odczyt tekstur PBR
    vec3  albedo    = pow(texture(albedoMap, fragTexCoord).rgb, vec3(2.2));
    float metallic  = texture(metallicMap, fragTexCoord).r;
    float roughness = clamp(texture(roughnessMap, fragTexCoord).r, 0.05, 1.0);

    // normalna z mapy normalnych przeksztalcona przez TBN
    vec3 tangentNormal = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
    vec3 N = normalize(TBN * tangentNormal);
    if (flipNormals && !gl_FrontFacing) N = -N;

    vec3 V = normalize(camPos - fragPos);
    vec3 L = normalize(lightPos - fragPos);
    vec3 H = normalize(V + L);

    // bazowa reflektywnosc (dielektryk = 0.04, metal = kolor albedo)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // BRDF Cook-Torrance
    float NDF = distributionGGX(N, H, roughness);
    float G   = geometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  numerator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3  specular    = numerator / denominator;

    // proporcje diffuse/specular
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    float shadow = ShadowCalculation(fragPosLightSpace, N, L);
    vec3 Lo = (kD * albedo / PI + specular) * lightColor * NdotL * (1.0 - shadow);

    // latarka
    if (spotOn) {
        vec3 lightToFrag = normalize(spotPos - fragPos);
        float theta = dot(lightToFrag, normalize(-spotDir));

        if (theta > spotCutoff) {
            float intensity = smoothstep(spotCutoff, spotCutoff + 0.05, theta);
            float distance = length(spotPos - fragPos);
            float attenuation = 1.0 / (1.0 + 0.04 * distance + 0.01 * (distance * distance));

            vec3 spotDiffuse = max(dot(N, lightToFrag), 0.0) * spotColor;
            Lo += albedo * spotDiffuse * intensity * attenuation * 3.0;
        }
    }

    // ambient podwodny
    vec3 ambient = vec3(0.03) * albedo;
    vec3 color   = ambient + Lo;

    // tone mapping Reinharda + korekcja gamma
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    // mgla podwodna
    float dist      = length(fragPos - camPos);
    float fogFactor = clamp(exp(-dist * 0.04), 0.0, 1.0);
    vec3  fogColor  = vec3(0.0, 0.15, 0.25);

    outColor = vec4(mix(fogColor, color, fogFactor), 1.0);
}
