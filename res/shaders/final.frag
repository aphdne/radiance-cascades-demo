#version 330 core

out vec4 fragColor;

uniform vec2      uResolution;
uniform sampler2D uCanvas;

void main() {
  fragColor = texture(uCanvas, gl_FragCoord.xy/textureSize(uCanvas, 0));
}
