#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aCoo;

out vec2 coordinates;

void main() {
    coordinates = aCoo;
    gl_Position = vec4(aPos, 0.0, 1.0);
}