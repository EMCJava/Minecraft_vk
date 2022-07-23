#version 450

layout(location = 0) out vec3 fragTexCoord;

layout(location = 0) in ivec3 inPosition;
layout(location = 1) in vec3 inCoor_ColorIntensity;

layout(binding  = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 highlightCoordinate;
    float time;
} ubo;

void main() {

    gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);

    vec3 diffVec = inPosition - ubo.highlightCoordinate;
    if (diffVec.x >= 0 && diffVec.x <= 1 && diffVec.y >= 0 && diffVec.y <= 1 && diffVec.z >= 0 && diffVec.z <= 1) {
        fragTexCoord = vec3(inCoor_ColorIntensity.xy, inCoor_ColorIntensity.z * mod(ubo.time, 1.0));
    } else {
        fragTexCoord = inCoor_ColorIntensity;
    }
}