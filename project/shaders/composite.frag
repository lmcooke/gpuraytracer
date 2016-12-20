#version 400 core

in vec2 uv;

uniform sampler2D tex;

out vec4 fragColor;

void main(){

    // Sample the texture "tex" at the given UV-coordinates.
    fragColor = texture(tex, uv);
}
