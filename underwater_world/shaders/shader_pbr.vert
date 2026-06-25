#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

uniform mat4 transformation;
uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix; // macierz cieni

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragTexCoord;
out mat3 TBN;
out vec4 fragPosLightSpace; // wyjscie do wyliczania cieni

void main()
{
    vec4 worldPosition = modelMatrix * vec4(vertexPosition, 1.0);
    gl_Position = transformation * vec4(vertexPosition, 1.0);

    fragPos = worldPosition.xyz;
    fragNormal = normalize((modelMatrix * vec4(vertexNormal, 0.0)).xyz);
    fragTexCoord = vertexTexCoord;

    vec3 T = normalize((modelMatrix * vec4(vertexTangent, 0.0)).xyz);
    vec3 B = normalize((modelMatrix * vec4(vertexBitangent, 0.0)).xyz);
    vec3 N = normalize((modelMatrix * vec4(vertexNormal, 0.0)).xyz);
    TBN = mat3(T, B, N);

    // obliczanie pozycji fragmentu w przestrzeni światła dla cieniowania
    fragPosLightSpace = lightSpaceMatrix * worldPosition; 
}