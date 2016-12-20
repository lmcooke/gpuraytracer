#version 400 core

in vec3 texCoords;
uniform samplerCube cube;
out vec4 fragColor;

void main(){
    fragColor = vec4(texture(cube, texCoords).bgr, 1.0);
}
