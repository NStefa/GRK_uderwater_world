#version 430 core

layout(location = 0) in vec3 vertexPosition;  
layout(location = 1) in vec3 vertexNormal;  
layout(location = 2) in vec2 vertexTexCoord;   

uniform mat4 transformation;  
uniform mat4 modelMatrix;   

out vec2 texCoord;  
out vec3 worldNormal; 
out vec3 worldPos;   

void main()
{
    vec4 worldPosition = modelMatrix * vec4(vertexPosition, 1.0);

    // transform to clip space for rasterizer
    gl_Position = transformation * vec4(vertexPosition, 1.0);

    texCoord = vertexTexCoord;

    // w=0 because normal is a direction, not a point
    worldNormal = normalize((modelMatrix * vec4(vertexNormal, 0.0)).xyz);

    worldPos = worldPosition.xyz;
}