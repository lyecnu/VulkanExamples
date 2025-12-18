#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;

layout (binding = 0) uniform MVP {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
} mvp;

layout (location = 0) out vec3 fragColor;

void main()
{
    gl_Position = mvp.projectionMatrix * mvp.viewMatrix * mvp.modelMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
}