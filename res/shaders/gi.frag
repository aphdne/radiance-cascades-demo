#version 330 core

#define TWO_PI 6.2831853071795864769252867665590
#define EPS 0.0005
#define MAX_RAY_STEPS 128

out vec4 fragColor;

uniform sampler2D uDistanceField;
uniform sampler2D uSceneMap;
uniform sampler2D uLastFrame;

uniform int   uRayCount;
uniform int   uSrgb;
uniform int   uNoise;
uniform float uDecayRate;
uniform float uMixFactor;

/* this shader performs "radiosity-based GI" - see comments */

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

vec4 raymarch(vec2 uv, vec2 dir) {
  /*
   * Raymarching entails iteratively stepping a ray by a certain amount using a distance field. We recursively
   * read the ray's position against a distance field, which tells us how far away the nearest surface is. Once
   * the distance reading is below a certain value, which in this shader is the preprocessor macro EPS (0.0006)
   * we mark the ray as having collided with a surface.
   *
   * Once we have hit a surface we can sample the surface that we have hit in the scene and return it.
   */
  for (int i = 0; i < MAX_RAY_STEPS; i++) {
    float dist = texture(uDistanceField, uv).r;               // sample distance field
    uv += dir * dist; // march our ray (divided by our aspect ratio so no skewed directions)

    // skip UVs outside of the window
    if (uv.xy != clamp(uv.xy, 0.0, 1.0))
      break;

    if (dist < EPS) // surface hit
	    return vec4(
        // here we're mixing the scene map with the previous frame
        mix(
          texture(uSceneMap,  uv).rgb,
          max(
            // we pick the maximum of either the last frame at `uv` or at the pixel right before `uv`
            // the picked texture is then multiplied by the decay rate so that lighting is not infinitely added
            // we also need to convert the texture from sRGB to linear colour space
            srgb_to_lin(texture(uLastFrame, vec2(uv.x, -uv.y)).rgb),
            srgb_to_lin(texture(uLastFrame, vec2(uv.x, -uv.y) - (dir * (1.0/textureSize(uSceneMap, 0)))).rgb) * uDecayRate
          ),
          uMixFactor
        ), 1.0);
  }
  return vec4(0.0);
}

// sourced from https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
float rand(vec2 n) {
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float noise(vec2 p){
	vec2 ip = floor(p);
	vec2 u = fract(p);
	u = u*u*(3.0-2.0*u);

	float res = mix(
		mix(rand(ip),rand(ip+vec2(1.0,0.0)),u.x),
		mix(rand(ip+vec2(0.0,1.0)),rand(ip+vec2(1.0,1.0)),u.x),u.y);
	return res*res;
}

void main() {
 /*
  * To calculate the radiance value of a pixel we cast `uRayCount` amount of rays in all directions
  * and add all of the resulting samples up, then divide by uRayCount
  */
  vec2 resolution = textureSize(uSceneMap, 0);
  vec2 fragCoord = gl_FragCoord.xy/resolution;

  float dist = texture(uDistanceField, fragCoord).r;
  float n = 0.0; // noise
  if (uNoise == 1) n = noise(fragCoord.xy * 2000);
  vec4 radiance = texture(uSceneMap, fragCoord);

  if (dist >= EPS) { // if we're not already in a wall
    // cast rays angularly with equal angles between them
    for (float i = 0.0; i < TWO_PI; i += TWO_PI / uRayCount) {
      float angle = i + n;
      vec4 hitcol = raymarch(fragCoord, vec2(cos(angle) * resolution.y/resolution.x, sin(angle)));
      radiance += hitcol;
    }
    radiance /= uRayCount;
  }

  fragColor = vec4((uSrgb == 1) ? lin_to_srgb(radiance.rgb) : radiance.rgb, 1.0);
}
