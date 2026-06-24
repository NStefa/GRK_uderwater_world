#version 430 core

in vec2 texCoord;
in vec3 worldNormal;
in vec3 worldPos;
in vec3 worldTangent;
in vec3 worldBitangent;

uniform sampler2D flowMap;
uniform sampler2D colorTexture;   // statyczny kolor skaly
uniform sampler2D normalMap;      // flow-distorted normalna

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
    vec2 flow = texture(flowMap, flowUV).rg * 2.0 - 1.0;

    float phase0 = fract(time * speed);
    float phase1 = fract(time * speed + 0.5);

    vec2 uv0 = texCoord - flow * phase0 * flowScale;
    vec2 uv1 = texCoord - flow * phase1 * flowScale;

    float blend = abs(phase0 * 2.0 - 1.0);

    // kolor statyczny - nie porusza sie z flowmapa
    vec4 texColor = texture(colorTexture, texCoord);

    // normalna flow-distorted - porusza sie jak woda po skale
    vec3 n0 = texture(normalMap, uv0).rgb * 2.0 - 1.0;
    vec3 n1 = texture(normalMap, uv1).rgb * 2.0 - 1.0;
    vec3 tangentNormal = normalize(mix(n0, n1, blend));

    mat3 TBN = mat3(
        normalize(worldTangent),
        normalize(worldBitangent),
        normalize(worldNormal)
    );
    vec3 normal = normalize(TBN * tangentNormal);

    vec3 view = normalize(cameraPos - worldPos);
    vec3 refl = reflect(-normalize(lightDir), normal);

    float diff = max(dot(normal, normalize(lightDir)), 0.0);
    float spec = pow(max(dot(view, refl), 0.0), 32.0);

    vec3 ambient  = 0.2 * lightColor;
    vec3 diffuse  = diff * lightColor;
    vec3 specular = 0.3 * spec * lightColor;

    vec3 litColor = (ambient + diffuse + specular) * texColor.rgb;

    float dist      = length(worldPos - cameraPos);
    float fogFactor = clamp(exp(-dist * 0.04), 0.0, 1.0);
    vec3  fogColor  = vec3(0.0, 0.15, 0.25);

    outColor = vec4(mix(fogColor, litColor, fogFactor), texColor.a);
}
