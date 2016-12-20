#version 400 core

in vec3 position;
uniform mat4 projection;
uniform mat4 view;

out vec3 texCoords;

void main() {
    texCoords = position;
    gl_Position = projection * view * vec4(position, 1.0);
}
