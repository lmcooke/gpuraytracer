#version 400 core

in vec2 uv;

uniform sampler2D tex;

out vec4 fragColor;

void main(){
    // QImage is bgra, so swap B and R
    fragColor = vec4(texture(tex, uv).bgr, 1.0);
}
