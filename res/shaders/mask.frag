#version 330 core

#define N 500.0

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D uOcclusionMask;
uniform vec2 uResolution;
uniform vec2 uLightPos;
uniform float uTime;

// from inigo quilez - https://iquilezles.org/
vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float terrain(vec2 p)
{
  //return step(0.25, texture(uOcclusionMask, p).x); // hard shadows
  return mix(0.95, 1.0, step(0.25, texture(uOcclusionMask, p).x)); // smooth shadows
}

void main() {
  vec2 p = fragTexCoord;

  vec2 l = uLightPos/uResolution;

  vec2 d = p - l;

  float b = 1.0;

  for(float i = 0.0; i < N; i++)
  {
    float t = i / N;
    float h = terrain(mix(p, l, t));
    b *= h;
  }

  // radial gradient around cursor
  b *= 1.0 - smoothstep(0.0, 0.5, length(d));

  finalColor = vec4(b * step(0.25, texture(uOcclusionMask, fragTexCoord)).xxx * hsv2rgb(vec3(uTime, 1.0f, 1.0f)), 1.0);
}
