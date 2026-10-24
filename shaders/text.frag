#version 450

layout(binding = 0) uniform sampler2D fontAtlas;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main() {
    float intensity = texture(fontAtlas, fragTexCoord).r;
    if (intensity < 0.5) {
        discard;
    }
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
}
