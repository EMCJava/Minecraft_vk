#version 450

layout(location = 0) in vec2 fragTexCoordBegin;
layout(location = 1) in vec2 accumulatedfragTexCoord;
layout(location = 2) in float visibility;
layout(location = 3) in float highlight;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

const float textureResolution = 16;

void main() {

    outColor = textureLod(texSampler, mod(accumulatedfragTexCoord, textureResolution) + fragTexCoordBegin, 0) * vec4(highlight, highlight, highlight, 1);
    outColor = mix(vec4(13.0 / 256, 129.0 / 256, 168.0 / 256, 0.5), outColor, visibility);
}