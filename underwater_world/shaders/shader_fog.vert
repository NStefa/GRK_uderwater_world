#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float time;
uniform float wiggleScale;

out vec2 texCoord;
out vec3 worldPos;
out vec3 worldNormal;

void main()
{
    vec3 pos = vertexPosition;
    
    float tailWeight = max(0.0, -pos.y + 5.0) * 0.05; 
    
    vec4 wPos = model * vec4(pos, 1.0);
    worldPos = wPos.xyz;
    worldNormal = normalize((model * vec4(vertexNormal, 0.0)).xyz);
    texCoord = vertexTexCoord;
    
    gl_Position = projection * view * wPos;
}