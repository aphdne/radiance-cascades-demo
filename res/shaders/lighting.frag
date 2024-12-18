#version 330 core

struct Light {
  vec2  position;
  vec3  color;
  float size;
};

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D uOcclusionMask;
uniform float uTime;
uniform int uLightsAmount;
uniform vec2 uResolution;
uniform vec2 uPlayerLocation;

uniform int uApple;
uniform int uSmoothShadows;
uniform int uCascadeAmount;
uniform int uViewing;

uniform Light lights[64];

// sourced from https://gist.github.com/companje/29408948f1e8be54dd5733a74ca49bb9
float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

// this technique is sourced from https://www.shadertoy.com/view/tddXzj
float terrain(vec2 p, vec2 normalisedLightPos, int smoothShadows)
{
  if (smoothShadows == 1) {
    if (uApple == 0) return mix(map(distance(uResolution * normalisedLightPos, gl_FragCoord.xy), 0.0, uResolution.x, 0.9, 1.0), 1.0, step(0.25, texture(uOcclusionMask, p).x));
  }
  return step(0.25, texture(uOcclusionMask, p).x); // hard shadows
}

void main() {
  vec3 result = vec3(0.0);

  for (int i = 0; i < uLightsAmount; i++) {
    vec2 normalisedLightPos = lights[i].position/uResolution;
    float brightness = 1.0;

    for (float j = 0.0; j < uCascadeAmount; j++) {
      float t = j / uCascadeAmount;
      float h = terrain(mix(fragTexCoord, normalisedLightPos, t), normalisedLightPos, uSmoothShadows);
      brightness *= h;
    }

    // radial gradient - adapted from https://www.shadertoy.com/view/4tjSWh

    if (uApple == 1) {
      brightness *= 1.0 - smoothstep(0.0, 0.5, length(fragTexCoord - normalisedLightPos));
    } else {
      brightness *= 1.0 - distance(uResolution.xy * vec2(normalisedLightPos.x, 1.0 - normalisedLightPos.y), gl_FragCoord.xy) * 2/lights[i].size;
    }

    if (brightness > 0) result += brightness * lights[i].color;
  }

  if (uViewing == 1) {
    vec2 normalisedPlayerPos = uPlayerLocation/uResolution;
    vec3 visible = vec3(1.0);

    for (float j = 0.0; j < uCascadeAmount; j++) {
      float t = j / uCascadeAmount;
      float h = terrain(mix(fragTexCoord, normalisedPlayerPos, t), normalisedPlayerPos, 0);
      visible *= h;
    }

    visible = mix(texture(uOcclusionMask, fragTexCoord).xyz * 0.015, result.xyz, visible);

    gl_FragColor = vec4(visible, 1.0f);
  } else {
    // combine light result w/ underlying occlusion mask texture
    gl_FragColor = vec4(result * step(0.25, texture(uOcclusionMask, fragTexCoord)).xxx, 1.0f);
  }
}
