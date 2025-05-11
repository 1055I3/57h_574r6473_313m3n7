#version 330 core

in vec2 coordinates;

uniform sampler2D image;
uniform bool blurToggle;

out vec4 fragColor;

void main() {
    const float weight[8] = float[](0.196387, 0.174566, 0.122196, 0.066652, 0.027772, 0.008545, 0.001831, 0.000244);
    vec2 texOffset = 4.0 / textureSize(image, 0);
    vec3 result = texture(image, coordinates).rgb * weight[0];

    if (blurToggle) {
        for (int i = 1; i < 8; ++i) {
            result += texture(image, coordinates + vec2(texOffset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, coordinates - vec2(texOffset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else {
        for (int i = 1; i < 8; ++i) {
            result += texture(image, coordinates + vec2(0.0, texOffset.y * i)).rgb * weight[i];
            result += texture(image, coordinates - vec2(0.0, texOffset.y * i)).rgb * weight[i];
        }
    }

    fragColor = vec4(result, 1.0);
}