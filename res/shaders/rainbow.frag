#version 330 core

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float uTime;

out vec4 finalColor;

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
  //finalColor = vec4(fragTexCoord.x, fragTexCoord.y, 0.0, 1.0);
  finalColor = vec4(hsv2rgb(vec3(uTime, 1.0f, 1.0f)), 1.0f);
}
