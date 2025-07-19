#version 430 core

uniform sampler2D outputTexture;

out vec4 FragColor;
in vec2 texCoords;

void main() {
    FragColor = texture(outputTexture, texCoords);
}