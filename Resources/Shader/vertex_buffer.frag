#version 450

layout(location = 0) in vec2 fragTexCoordBegin;
layout(location = 1) in vec2 accumulatedfragTexCoord;
layout(location = 2) in vec3 selectionDiff;
layout(location = 3) in float visibility;
layout(location = 4) in float colorIntensity;
layout(location = 5) in float time;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

const float textureResolution = 16;

void main() {

    vec4 colorModifier = vec4(colorIntensity, colorIntensity, colorIntensity, 1);

    // selection highlight
    if (selectionDiff.x >= 0 && selectionDiff.x <= 1 && selectionDiff.y >= 0 && selectionDiff.y <= 1 && selectionDiff.z >= 0 && selectionDiff.z <= 1) {
        colorModifier *= sin(time * 3) * 0.3 + 0.7;
    }

    outColor = textureLod(texSampler, mod(accumulatedfragTexCoord, textureResolution) + fragTexCoordBegin, 0) * colorModifier;
    outColor = mix(vec4(13.0 / 256, 129.0 / 256, 168.0 / 256, 0.5), outColor, visibility);
}