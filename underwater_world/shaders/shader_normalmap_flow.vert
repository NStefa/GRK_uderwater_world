#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

uniform mat4 modelMatrix;
uniform mat4 transformation;

uniform mat4 lightSpaceMatrix;

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;
out vec3 worldTangent;
out vec3 worldBitangent;

out vec4 fragPosLightSpace;

void main()
{
    worldPos = (modelMatrix * vec4(vertexPosition, 1.0)).xyz;
    worldNormal = (modelMatrix * vec4(vertexNormal, 0.0)).xyz;
    worldTangent = (modelMatrix * vec4(vertexTangent, 0.0)).xyz;
    worldBitangent = (modelMatrix * vec4(vertexBitangent, 0.0)).xyz;
    texCoord = vertexTexCoord;
    
    fragPosLightSpace = lightSpaceMatrix * vec4(worldPos, 1.0);

    gl_Position = transformation * vec4(vertexPosition, 1.0);
}