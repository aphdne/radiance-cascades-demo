#version 330 core

out vec4 fragColor;

uniform vec2      uResolution;
uniform sampler2D uCanvas;

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution;
  fragColor = texture(uCanvas, vec2(fragCoord.x, fragCoord.y));
}
