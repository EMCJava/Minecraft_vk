#version 450

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {

    outColor = textureLod(texSampler, fragTexCoord.xy, 0) * vec4(fragTexCoord.z, fragTexCoord.z, fragTexCoord.z, 1);
}