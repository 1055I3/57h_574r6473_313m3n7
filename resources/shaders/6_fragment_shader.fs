#version 330 core

in vec2 coordinates;

uniform sampler2D baseImage;
uniform sampler2D highlights;

out vec4 fragColor;

void main() {
    const float gamma = 1.4;
    vec3 screen = texture(baseImage, coordinates).rgb;
    vec3 bloom = texture(highlights, coordinates).rgb;
    vec3 outputImage = screen + bloom;
    outputImage = vec3(1.0) - exp(-outputImage * 0.9);
    outputImage = pow(outputImage, vec3(1.0 / gamma));
    fragColor = vec4(outputImage, 1.0);
}