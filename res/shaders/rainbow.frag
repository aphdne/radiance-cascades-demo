#version 330 core

in vec2 texCoords;

out vec4 fragColor;

uniform float uTime;

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
  fragColor = vec4(hsv2rgb(vec3(uTime, 1.0f, 1.0f)), 1.0f);
}
