#version 430 core
layout(location = 0) in vec3 vertexPosition;
uniform mat4 projection;
uniform mat4 view;
out vec3 texCoords;
void main()
{
    texCoords = vertexPosition;
    vec4 pos = projection * mat4(mat3(view)) * vec4(vertexPosition, 1.0);
    gl_Position = pos.xyww;
}