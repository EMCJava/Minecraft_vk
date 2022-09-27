#version 450

layout(location = 0) out vec2 fragTexCoordBegin;
layout(location = 1) out vec2 accumulatedfragTexCoord;
layout(location = 2) out vec3 selectionDiff;
layout(location = 3) out float visibility;
layout(location = 4) out float colorIntensity;
layout(location = 5) out float time;

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

    fragTexCoordBegin = inCoor_ColorIntensity.xy;
    accumulatedfragTexCoord = accumulatedTextureCoordinate;
    selectionDiff = inPosition - ubo.highlightCoordinate;// highlight selected block
    colorIntensity = inCoor_ColorIntensity.z;
    time = ubo.time;

    // not selecting
    if (ubo.highlightCoordinate.y < 0) {
        selectionDiff.x = -1;
    }

    // fog
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