#version 330 core

struct Light {
  vec2  position;
  vec3  color;
  float size;
};

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D uOcclusionMask;
uniform vec2 uResolution;
uniform float uTime;
uniform int uLightsAmount;
uniform int uApple;

uniform Light lights[64];

float terrain(vec2 p)
{
  return step(0.25, texture(uOcclusionMask, p).x); // hard shadows
  //return mix(0.8, 1.0, step(0.25, texture(uOcclusionMask, p).x)); // smooth shadows
}

void main() {
  vec3 lightResult = vec3(0.0);
  vec3 brightResult = vec3(0.0);

  float N = 512.0;
  if (uApple == 1) N = 128.0;

  for (int i = 0; i < uLightsAmount; i++) {
    vec2 normalisedLightPos = lights[i].position/uResolution;
    float brightness = 1.0;

    for (float j = 0.0; j < N; j++) {
      float t = j / N;
      float h = terrain(mix(fragTexCoord, normalisedLightPos, t));
      brightness *= h;
    }

    // radial gradient - adapted from https://www.shadertoy.com/view/4tjSWh

    if (uApple == 1) {
      brightness *= 1.0 - smoothstep(0.0, 0.5, length(fragTexCoord - normalisedLightPos));
    } else {
      brightness *= 1.0 - distance(uResolution.xy * vec2(normalisedLightPos.x, 1.0 - normalisedLightPos.y), gl_FragCoord.xy) * 2/lights[i].size;
    }

    if (brightness > 0) brightResult += brightness * lights[i].color;
  }

  //finalColor = vec4(brightResult * step(0.25, texture(uOcclusionMask, fragTexCoord)).xxx, 1.0f);
  finalColor = vec4(brightResult, 1.0f);
}
