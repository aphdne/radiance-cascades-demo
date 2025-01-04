#version 330 core

#define VIEWER_OUT_OF_SIGHT_BRIGHTNESS 0.005
#define VIEWER_GRADIENT_RADIUS         1000

#define STATIC 0
#define SINE   1
#define SAW    2
#define NOISE  3

#define PI 3.1415926535897932384626433

struct Light {
  vec2  position;
  vec3  color;
  float radius;
  float timeCreated; // doubles dont exist in this GLSL version
  int   type;
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

// sourced from https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
float hash(float n) { return fract(sin(n) * 1e4); }

float noise(float x) {
	float i = floor(x);
	float f = fract(x);
	float u = f * f * (3.0 - 2.0 * f);
	return mix(hash(i), hash(i + 1.0), u);
}

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

vec3 calcVisibility(Light l, bool smoothShadows) {
  // no need to bother calculating light if the light value will be overridden by the gradient anyway
  if (distance(fragTexCoord*uResolution, l.position) > l.radius) return vec3(0);

  float brightness = 1.0;

  vec2 normalisedPos = l.position/uResolution;

  for (float j = 0.0; j < uCascadeAmount; j++) {
    float t = j / uCascadeAmount;
    float h = terrain(mix(fragTexCoord, normalisedPos, t), l.position, smoothShadows);
    brightness *= h;
  }

  // generate a seed from 1-3 based on the light's colour as the light's colour cannot change
  float seed = (l.color.r + l.color.g + l.color.b) / 1.5 + 1;

  // radial gradient - adapted from https://www.shadertoy.com/view/4tjSWh
  brightness *= 1.0 - distance(vec2(l.position.x, uResolution.y - l.position.y), gl_FragCoord.xy) * 1/l.radius;
  if      (l.type == SINE)  brightness *= abs(sin(uTime*seed/2 - l.timeCreated));
  else if (l.type == SAW)   brightness *= (abs(sin(uTime*seed - l.timeCreated)) > 0.5 ) ? 0 : 1;
  else if (l.type == NOISE) brightness *= noise(uTime*seed - l.timeCreated);

  return (brightness > 0) ? brightness * l.color : vec3(0);
}

void main() {
  vec3 result = vec3(0.0);

  for (int i = 0; i < uLightsAmount; i++) {
    result += calcVisibility(lights[i], true);
  }

  if (uViewing == 1) {
    Light player;
    player.position = uPlayerLocation;
    player.radius = VIEWER_GRADIENT_RADIUS;
    player.color = vec3(1.0);
    player.timeCreated = 0;
    player.type = STATIC;
    fragColor = vec4(
      mix(
        texture(uOcclusionMask, fragTexCoord).xyz * VIEWER_OUT_OF_SIGHT_BRIGHTNESS,
        result.xyz,
        calcVisibility(player, false)
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
