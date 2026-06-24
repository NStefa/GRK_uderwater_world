#version 430 core

in vec2 texCoord;
in vec3 worldNormal;
in vec3 worldPos;
in vec3 worldTangent;
in vec3 worldBitangent;

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

out vec4 outColor;

void main()
{
    vec2 flowUV = texCoord * flowMapScale;
    // Remap [0,1] -> [-1,1]: 0.5 = no flow, 0.0 = max left, 1.0 = max right
    vec2 flow = texture(flowMap, flowUV).rg * 2.0 - 1.0;

    // Two phases offset by 0.5 to avoid a visible snap each cycle
    float phase0 = fract(time * speed);
    float phase1 = fract(time * speed + 0.5);

    vec2 uv0 = texCoord - flow * phase0 * flowScale;
    vec2 uv1 = texCoord - flow * phase1 * flowScale;

    // Triangle wave blend: smoothly crossfades the two phases instead of snapping
    float blend = abs(phase0 * 2.0 - 1.0);

    // kolor flow-distorted - przesuwa sie razem z normalną
    vec4 c0 = texture(colorTexture, uv0);
    vec4 c1 = texture(colorTexture, uv1);
    vec4 texColor = mix(c0, c1, blend);

    // sample normal map with same flow-distorted UVs and blend phases
    vec3 n0 = texture(normalMap, uv0).rgb * 2.0 - 1.0;
    vec3 n1 = texture(normalMap, uv1).rgb * 2.0 - 1.0;
    vec3 tangentNormal = normalize(mix(n0, n1, blend));

    // TBN: tangent space → world space
    mat3 TBN = mat3(
        normalize(worldTangent),
        normalize(worldBitangent),
        normalize(worldNormal)
    );
    vec3 normal = normalize(TBN * tangentNormal);

    // Phong lighting (ambient + diffuse)
    float diff = max(dot(normal, normalize(lightDir)), 0.0);
    vec3 ambient = 0.2 * lightColor;
    vec3 diffuse = diff * lightColor * 0.5;
    vec3 litColor = (ambient + diffuse) * texColor.rgb;

    // Exponential underwater fog: exp(-dist * density), 1=no fog, 0=full fog
    float dist = length(worldPos - cameraPos);
    float fogFactor = clamp(exp(-dist * 0.04), 0.0, 1.0);
    vec3 fogColor = vec3(0.0, 0.15, 0.25);

    outColor = vec4(mix(fogColor, litColor, fogFactor), texColor.a);
}
