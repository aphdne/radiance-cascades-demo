#version 330 core

#define VIEWER_OUT_OF_SIGHT_BRIGHTNESS 0.015
#define VIEWER_GRADIENT_RADIUS         450

struct Light {
  vec2  position;
  vec3  color;
  float radius;
};

in vec2 fragTexCoord;

out vec4 fragColor;

// data
uniform Light lights[128];
uniform int uLightsAmount;
uniform sampler2D uOcclusionMask;
uniform float uTime;
uniform vec2 uResolution;
uniform vec2 uPlayerLocation;

// config
uniform int uCascadeAmount; // the amount of cascades primarily affects performance & light leakage. Thicker walls = less cascades needed, thinner walls = more cascades needed
uniform int uViewing;

// sourced from https://gist.github.com/companje/29408948f1e8be54dd5733a74ca49bb9
float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

// this technique is sourced from https://www.shadertoy.com/view/tddXzj
float terrain(vec2 p, vec2 position, bool smoothShadows)
{
  if (smoothShadows) {
    // increased opacity closer to the light source
    return mix(
      map(
        distance(position, gl_FragCoord.xy),
        0.0,
        ((uResolution.x < uResolution.y) ? uResolution.y : uResolution.x) * 500,
        0.9,
        1.0
      ),
      1.0,
      step(0.25, texture(uOcclusionMask, p).x)
    );
  }
  return step(0.25, texture(uOcclusionMask, p).x); // hard shadows
}

vec3 calcVisibility(vec2 position, float gradientRadius = 300, vec3 rgb = vec3(1.0), bool smoothShadows = true) {
  // no need to bother calculating light if the light value will be overridden by the gradient anyway
  if (distance(fragTexCoord*uResolution, position) > gradientRadius) return vec3(0);

  float brightness = 1.0;

  vec2 normalisedPos = position/uResolution;

  for (float j = 0.0; j < uCascadeAmount; j++) {
    float t = j / uCascadeAmount;
    float h = terrain(mix(fragTexCoord, normalisedPos, t), position, smoothShadows);
    brightness *= h;
  }

  // radial gradient - adapted from https://www.shadertoy.com/view/4tjSWh
  brightness *= 1.0 - distance(vec2(position.x, uResolution.y - position.y), gl_FragCoord.xy) * 1/gradientRadius;

  return (brightness > 0) ? brightness * rgb : vec3(0);
}

void main() {
  vec3 result = vec3(0.0);

  for (int i = 0; i < uLightsAmount; i++) {
    result += calcVisibility(lights[i].position, lights[i].radius, lights[i].color);
  }

  if (uViewing == 1) {
    fragColor = vec4(
      mix(
        texture(uOcclusionMask, fragTexCoord).xyz * VIEWER_OUT_OF_SIGHT_BRIGHTNESS,
        result.xyz,
        calcVisibility(uPlayerLocation, VIEWER_GRADIENT_RADIUS, vec3(1.0), false)
      ),
      1.0
    );
  } else {
    // combine light result w/ underlying occlusion mask texture
    fragColor = vec4(
      result * step(0.25, texture(uOcclusionMask, fragTexCoord)).xxx,
      1.0
    );
  }
}
