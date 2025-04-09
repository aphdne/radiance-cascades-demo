#version 330 core

#define PI 3.141596
#define TWO_PI 6.2831853071795864769252867665590
#define TAU 0.0008
#define DECAY_RATE 1.1

out vec4 fragColor;

uniform sampler2D uDistanceField;
uniform sampler2D uSceneMap;
uniform sampler2D uLastFrame;

uniform vec2 uResolution;
uniform int  uRaysPerPx;
uniform int  uMaxSteps;

/* this shader performs "radiosity-based GI" - see comments */

vec4 raymarch(vec2 uv, vec2 dir) {
  /*
   * Raymarching entails iteratively stepping a ray by a certain amount using a distance field. We recursively
   * read the ray's position against a distance field, which tells us how far away the nearest surface is. Once
   * the distance reading is below a certain value, which in this shader is the preprocessor macro TAU (0.0006)
   * we mark the ray as having collided with a surface.
   *
   * Once we have hit a surface we can sample the surface that we have hit in the scene and return it.
   */
  for (int i = 0; i < uMaxSteps; i++) {
    float dist = texture(uDistanceField, uv).r;               // sample distance field
    uv += (dir * dist) / (uResolution.x/uResolution.y); // march our ray (divided by our aspect ratio so no skewed directions)

    // skip UVs outside of the window
    if (uv.x != clamp(uv.x,  0.0, 1.0) || uv.y != clamp(uv.y, 0.0, 1.0))
      break;

    if (dist < TAU) // surface hit#
      return max(texture(uLastFrame, uv), texture(uLastFrame, uv - (dir * (1.0/uResolution))) * DECAY_RATE);
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
  * To calculate the radiance value of a pixel we cast `uRaysPerPx` amount of rays in all directions
  * and add all of the resulting samples up, then divide by uRaysPerPx
  */
  vec2 fragCoord = gl_FragCoord.xy/uResolution;

  float dist = texture(uDistanceField, fragCoord).r;
  float noise = noise(fragCoord.xy * 2000);
  vec4 radiance = texture(uLastFrame, fragCoord);

  if (dist >= TAU) { // if we're not already in a wall
    // cast rays angularly with equal angles between them
    for (float i = 0.0; i < TWO_PI; i += TWO_PI / uRaysPerPx) {
      float angle = i;
      vec4 hitcol = raymarch(fragCoord, vec2(cos(angle) * uResolution.y/uResolution.x, sin(angle)));
      radiance += hitcol;
    }
    radiance /= uRaysPerPx;
  }

  fragColor = vec4(radiance.rgb, 1.0);
}
