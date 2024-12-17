#version 330 core

#define N             500.0 // amount of passes
#define GRADIENT_SIZE 400 // in pixels

in vec2 fragTexCoord;
in vec2 gl_FragCoord;

out vec4 finalColor;

uniform sampler2D uOcclusionMask;
uniform vec2 uResolution;
uniform vec2 uLightPos;
uniform float uTime;

float terrain(vec2 p)
{
  return step(0.25, texture(uOcclusionMask, p).x); // hard shadows
  //return mix(0.8, 1.0, step(0.25, texture(uOcclusionMask, p).x)); // smooth shadows
}

void main() {
  vec2 normalisedLightPos = uLightPos/uResolution;

  float brightness = 1.0;

  for(float i = 0.0; i < N; i++)
  {
    float t = i / N;
    float h = terrain(mix(fragTexCoord, normalisedLightPos, t));
    brightness *= h;
  }

  // radial gradient - adapted from https://www.shadertoy.com/view/4tjSWh
  brightness *= 1.0 - distance(uResolution.xy * vec2(normalisedLightPos.x, 1.0 - normalisedLightPos.y), gl_FragCoord.xy) * 2/GRADIENT_SIZE;

  vec3 col = vec3(1.0f);

  finalColor = vec4(brightness * col * step(0.25, texture(uOcclusionMask, fragTexCoord)).xxx, 1.0f);
}
