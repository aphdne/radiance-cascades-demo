#version 330 core

#define PI 3.141596
#define TWO_PI 6.2831853071795864769252867665590
#define EPS 0.0008
#define DECAY_RATE 1.1
#define FIRST_LEVEL uCascadeIndex == 0
#define LAST_LEVEL uCascadeIndex == uCascadeAmount

out vec4 fragColor;

uniform sampler2D uDistanceField;
uniform sampler2D uSceneMap;
uniform sampler2D uLastPass;

uniform vec2  uResolution;
uniform int   uBaseRayCount;
uniform int   uMaxSteps;
uniform float uPointA;
uniform float uPointB;
uniform int   uCascadeDisplayIndex;
uniform int   uCascadeIndex;
uniform int   uCascadeAmount;
uniform int   uSrgb;
uniform int   uTest;
uniform float uDecayRate;
uniform int uDisableMerging;
uniform float uBaseInterval;

struct probe {
  float spacing;       // probe amount per dimension e.g. 1, 2, 4, 16
  vec2 size;           // screen size of probe in screen-space coordinates e.g. 1.0x1.0, 0.5x0.5, etc.
  vec2 position;       // relative coordinates within encapsulating probe
  // vec2 center;         // centre of current probe
  vec2 rayPosition;
  float intervalStart;
  float intervalEnd;
  float rayCount;
};

probe get_probe_info(int index) {
  probe p;
  vec2 fragCoord = gl_FragCoord.xy/uResolution;

  // amount of probes in our current cascade
  // [1, 4, 16, 256, ...]
  float probeAmount = pow(uBaseRayCount, index);
  p.spacing = sqrt(probeAmount); // probe amount per dimension

  // screen size of a probe in our current cascade
  // [resolution/1, resolution/2, resolution/4,  resolution/16, ...]
  // [1.0x1.0,      0.5x0.05,     0.25x0.25,     0.0625x0.0625, ...]
  p.size = 1.0/vec2(p.spacing);

  // current position within a probe in our current cascade
  p.position = mod(fragCoord, p.size) * p.spacing;

  // centre of current probe
  // p.center = (p.position + vec2(0.5/uResolution)) * p.spacing / uResolution;

  p.rayCount = pow(uBaseRayCount, index+1); // angular resolution

  // calculate which group of rays we're calculating this pass
  p.rayPosition = floor(fragCoord / p.size);

  float a = uBaseInterval; // px
  p.intervalStart = (FIRST_LEVEL) ? 0.0 : a * pow(uBaseRayCount, uCascadeIndex) / max(uResolution.x, uResolution.y);
  p.intervalEnd = a * pow(uBaseRayCount, uCascadeIndex+1) / max(uResolution.x, uResolution.y);

  // float offset = 1.5/uResolution.x;
  // p.intervalStart += offset;
  // p.intervalEnd += offset;

  return p;
}

// altered raymarching function; only calculates coordinates with a distance between a and b
vec4 radiance_interval(vec2 uv, vec2 dir, float a, float b) {
  uv += a * dir;
  float travelledDist = a;
  for (int i = 0; i < uMaxSteps; i++) {
    float dist = texture(uDistanceField, uv).r;         // sample distance field
    uv += (dir * dist); // march our ray

    // skip UVs outside of the window
    if (uv.x != clamp(uv.x,  0.0, 1.0) || uv.y != clamp(uv.y, 0.0, 1.0))
      break;

    // surface hit
    if (dist < EPS)
      return max(texture(uSceneMap, uv), texture(uSceneMap, uv - (dir * (1.0/uResolution))) * uDecayRate);
      // return texture(uSceneMap, uv);

    travelledDist += dist;
    if (travelledDist >= b)
      break;
  }
  return vec4(0.0);
}

vec3 lin_to_srgb(vec3 color)
{
   vec3 x = color.rgb * 12.92;
   vec3 y = 1.055 * pow(clamp(color.rgb, 0.0, 1.0), vec3(0.4166667)) - 0.055;
   vec3 clr = color.rgb;
   clr.r = (color.r < 0.0031308) ? x.r : y.r;
   clr.g = (color.g < 0.0031308) ? x.g : y.g;
   clr.b = (color.b < 0.0031308) ? x.b : y.b;
   return clr.rgb;
}

void main() {
  vec4 radiance = vec4(0.0);

  probe p = get_probe_info(uCascadeIndex);

  float baseIndex = float(uBaseRayCount) * (p.rayPosition.x + (p.spacing * p.rayPosition.y));

  for (float i = 0.0; i < uBaseRayCount; i++) {
    float index = baseIndex + i;
    float angle = (index / p.rayCount) * TWO_PI;

    vec4 deltaRadiance = vec4(0.0);

    deltaRadiance += radiance_interval(
      p.position,
      vec2(cos(angle) * min(uResolution.x, uResolution.y) / max(uResolution.x, uResolution.y), sin(angle)),
      p.intervalStart,
      p.intervalEnd
    );

    if (!(LAST_LEVEL) && deltaRadiance.a == 0.0 && uDisableMerging != 1.0) {
      probe up = get_probe_info(uCascadeIndex+1);

      up.position = vec2(
        mod(index, up.spacing), floor(index / up.spacing)
      ) * up.size;

      #define PIXEL vec2(1.0)/uResolution
      vec2 offset = p.position / up.spacing;
      offset = clamp(offset, PIXEL, up.size - PIXEL);
      vec2 uv = up.position + offset;

      deltaRadiance += texture(uLastPass, uv);
    }
    radiance += deltaRadiance;
  }
  radiance /= uBaseRayCount;
  // radiance *= 1.1;

  if (uCascadeIndex < uCascadeDisplayIndex) radiance = vec4(vec3(texture(uLastPass, gl_FragCoord.xy/uResolution)), 1.0);

  fragColor = vec4((FIRST_LEVEL && (uSrgb == 1 && uTest == 1)) ? radiance.rgb*1.5 : radiance.rgb, 1.0);
  // if (uTest == 1) {
  //   fragColor = texture(uSceneMap, gl_FragCoord.xy/uResolution);
  // }
}
