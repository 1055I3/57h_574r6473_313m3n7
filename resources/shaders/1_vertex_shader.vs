#version 330 core

layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNor;
layout (location=2) in vec2 aCoo;

out vec3 normals;
out vec2 coordinates;
out vec3 fragPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    normals = aNor;
    coordinates = aCoo;
    fragPosition = vec3(model * vec4(aPos, 1.0));

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}