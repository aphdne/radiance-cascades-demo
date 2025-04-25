#version 330 core

#define TWO_PI 6.2831853071795864769252867665590
#define EPS 0.0005
#define FIRST_LEVEL uCascadeIndex == 0
#define LAST_LEVEL uCascadeIndex == uCascadeAmount
#define MAX_RAY_STEPS 128

out vec4 fragColor;

uniform sampler2D uDistanceField;
uniform sampler2D uSceneMap;
uniform sampler2D uDirectLighting;
uniform sampler2D uLastPass;

uniform vec2  uResolution;
uniform int   uBaseRayCount;
uniform int   uCascadeDisplayIndex;
uniform int   uCascadeIndex;
uniform int   uCascadeAmount;
uniform int   uSrgb;
uniform float uPropagationRate;
uniform int   uDisableMerging;
uniform float uBaseInterval;
uniform float uMixFactor;

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
  p.intervalStart = (FIRST_LEVEL) ? 0.0 : a * pow(uBaseRayCount, index) / min(uResolution.x, uResolution.y);
  p.intervalEnd = a * pow(uBaseRayCount, index+1) / min(uResolution.x, uResolution.y);

  return p;
}

// sourced from https://gist.github.com/Reedbeta/e8d3817e3f64bba7104b8fafd62906dfj
vec3 lin_to_srgb(vec3 rgb)
{
  return mix(1.055 * pow(rgb, vec3(1.0 / 2.4)) - 0.055,
             rgb * 12.92,
             lessThanEqual(rgb, vec3(0.0031308)));
}

vec3 srgb_to_lin(vec3 rgb)
{
  return mix(pow((rgb + 0.055) * (1.0 / 1.055), vec3(2.4)),
             rgb * (1.0/12.92),
             lessThanEqual(rgb, vec3(0.04045)));
}


// altered raymarching function; only calculates coordinates with a distance between a and b
vec4 radiance_interval(vec2 uv, vec2 dir, float a, float b) {
  uv += a * dir;
  float travelledDist = a;
  for (int i = 0; i < MAX_RAY_STEPS; i++) {
    float dist = texture(uDistanceField, uv).r;         // sample distance field
    uv += (dir * dist); // march our ray

    // skip UVs outside of the window
    if (uv.xy != clamp(uv.xy, 0.0, 1.0))
      break;

    // surface hit
    if (dist < EPS) {
      if (uMixFactor != 0) {
        return vec4(
            mix(
              texture(uSceneMap, uv).rgb,
              max(
                texture(uDirectLighting, vec2(uv.x, -uv.y)).rgb,
                texture(uDirectLighting, vec2(uv.x, -uv.y) - (dir * (1.0/uResolution))).rgb * uPropagationRate
              ),
              uMixFactor
            ),
            1.0);
      }
      return vec4(texture(uSceneMap, uv).rgb, 1.0); // this is a little redundant but produces a 2-3 fps loss when removed
    }

    travelledDist += dist;
    if (travelledDist >= b)
      break;
  }
  return vec4(0.0);
}

void main() {
  vec4 radiance = vec4(0.0);

  probe p = get_probe_info(uCascadeIndex);
  probe up = get_probe_info(uCascadeIndex+1);

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

    // merging
    if (!(LAST_LEVEL) && deltaRadiance.a == 0.0 && uDisableMerging != 1.0) {
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

  if (uCascadeIndex < uCascadeDisplayIndex) radiance = vec4(vec3(texture(uLastPass, gl_FragCoord.xy/uResolution)), 1.0);

  fragColor = vec4((FIRST_LEVEL && uSrgb == 1) ? lin_to_srgb(radiance.rgb) : radiance.rgb, 1.0);
  // if (uMixFactor != 0 ) fragColor = texture(uDirectLighting, gl_FragCoord.xy/uResolution);
}
