#version 430 core

in vec2 texCoord;
in vec3 worldNormal;
in vec3 worldPos;

uniform sampler2D flowMap;      // R=X direction, G=Z direction, range [0,1]
uniform sampler2D colorTexture;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 cameraPos;

uniform float time;
uniform float speed;
uniform float flowScale;

out vec4 outColor;

void main()
{
    // Sample flow map at low UV scale for broad, smooth flow regions
    vec2 flowUV = texCoord * 0.05;
    // Remap [0,1] -> [-1,1]: 0.5 = no flow, 0.0 = max left, 1.0 = max right
    vec2 flow = texture(flowMap, flowUV).rg * 2.0 - 1.0;

    // Two phases offset by 0.5 to avoid a visible snap each cycle
    float phase0 = fract(time * speed);
    float phase1 = fract(time * speed + 0.5);

    vec2 uv0 = texCoord - flow * phase0 * flowScale;
    vec2 uv1 = texCoord - flow * phase1 * flowScale;

    // Triangle wave blend: smoothly crossfades the two phases instead of snapping
    float blend = abs(phase0 * 2.0 - 1.0);

    vec4 color0 = texture(colorTexture, uv0);
    vec4 color1 = texture(colorTexture, uv1);
    vec4 texColor = mix(color0, color1, blend);

    // Phong lighting (ambient + diffuse)
    float diff = max(dot(normalize(worldNormal), normalize(lightDir)), 0.0);
    vec3 ambient = 0.3 * lightColor;
    vec3 diffuse = diff * lightColor;
    vec3 litColor = (ambient + diffuse) * texColor.rgb;

    // Exponential underwater fog: exp(-dist * density), 1=no fog, 0=full fog
    float dist = length(worldPos - cameraPos);
    float fogFactor = clamp(exp(-dist * 0.04), 0.0, 1.0);
    vec3 fogColor = vec3(0.0, 0.15, 0.25);

    outColor = vec4(mix(fogColor, litColor, fogFactor), texColor.a);
}
