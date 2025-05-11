#version 330 core

layout (location=0) out vec4 fragColor;
layout (location=1) out vec4 brightColor;

in vec2 coordinates;

uniform sampler2D texture_diffuse1;

void main() {
    fragColor = texture(texture_diffuse1, coordinates);

    float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (0.8 < brightness)
        brightColor = fragColor;
    else
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
}