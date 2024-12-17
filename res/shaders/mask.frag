#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D uOcclusionMask;
uniform vec2 uResolution;
uniform vec2 uMousePos;

void main() {
  vec2 p = fragTexCoord;//uResolution;

  finalColor = vec4(step(0.25, texture(uOcclusionMask, fragTexCoord)).xxx, 1.0);
}
