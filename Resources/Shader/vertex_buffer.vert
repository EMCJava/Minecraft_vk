#version 450

layout(location = 0) out vec2 fragTexCoordBegin;
layout(location = 1) out vec2 accumulatedfragTexCoord;
layout(location = 2) out float visibility;
layout(location = 3) out float highlight;

layout(location = 0) in ivec3 inPosition;
layout(location = 1) in vec3 inCoor_ColorIntensity;
layout(location = 2) in vec2 accumulatedTextureCoordinate;

layout(binding  = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 highlightCoordinate;
    float time;
} ubo;

const float fogDistance = 150;
const float density = 0.05;
const float gradient = 1.5;

void main() {

    const vec4 positionRelativeToCamera = ubo.view * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * positionRelativeToCamera;

    accumulatedfragTexCoord = accumulatedTextureCoordinate;
    fragTexCoordBegin = inCoor_ColorIntensity.xy;
    vec3 diffVec = inPosition - ubo.highlightCoordinate;
    if (ubo.highlightCoordinate.y >= 0 && diffVec.x >= 0 && diffVec.x <= 1 && diffVec.y >= 0 && diffVec.y <= 1 && diffVec.z >= 0 && diffVec.z <= 1) {
        highlight = inCoor_ColorIntensity.z * mod(ubo.time, 1.0);
    } else {
        highlight = inCoor_ColorIntensity.z;
    }

    float pointDistance = length(positionRelativeToCamera.xyz);

    if (pointDistance > fogDistance)
    {
        pointDistance -= fogDistance;
        visibility = exp(-pow(pointDistance*density, gradient));
        visibility = clamp(visibility, 0.0, 1.0);
    } else {

        visibility = 1.0;
    }
}