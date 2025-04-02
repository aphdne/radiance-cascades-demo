#version 330 core

#define PI 3.141596
#define TWO_PI 6.2831853071795864769252867665590
#define TAU 0.0008
#define DECAY_RATE 1.3

out vec4 fragColor;

uniform vec2  uResolution;
uniform int   uRaysPerPx;
uniform int   uMaxSteps;
uniform float uPointA;
uniform float uPointB;
uniform float uLinearResolution;

uniform sampler2D uDistanceField;
uniform sampler2D uSceneMap;
uniform sampler2D uLastPass;

// altered raymarching function; only returns coordinates with a distance between a and b
vec2 radiance_interval(vec2 uv, vec2 dir, float a, float b) {
  uv += a * dir;
  float travelledDist = a;
  for (int i = 0; i < uMaxSteps; i++) {
    float dist = texture(uDistanceField, uv).r;         // sample distance field
    uv += (dir * dist); // march our ray (divided by our aspect ratio so no skewed directions)

    // skip UVs outside of the window
    if (uv.x != clamp(uv.x,  0.0, 1.0) || uv.y != clamp(uv.y, 0.0, 1.0))
      break;

    // surface hit
    if (dist < TAU)
      return uv;

    travelledDist += dist;
    if (travelledDist >= b)
      break;
  }
  return vec2(0.0);
}

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution;

  float dist = texture(uDistanceField, fragCoord).r;
  vec3 radiance = texture(uSceneMap, fragCoord).rgb;

  vec2 uv = floor(gl_FragCoord.xy / uLinearResolution) * uLinearResolution / uResolution;

  if (dist >= TAU) { // if we're not already in a wall
    // cast rays angularly with equal angles between them
    for (float i = 0.0; i < TWO_PI; i += TWO_PI / uRaysPerPx) {
      float angle = i + 0.5;
      vec2 hitpos = radiance_interval(uv, vec2(cos(angle) * uResolution.y/uResolution.x, sin(angle)), uPointA, uPointB);
      radiance += texture(uSceneMap, hitpos).rgb;
    }
    radiance /= uRaysPerPx;
  }

  fragColor = vec4((radiance + texture(uLastPass, fragCoord).rgb) / 2, 1.0);
}
