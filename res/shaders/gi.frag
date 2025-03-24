#version 330 core

#define PI 3.141596
#define TWO_PI 6.2831853071795864769252867665590
#define TAU 0.0008
#define DECAY_RATE 1.2

out vec4 fragColor;

uniform vec2 uResolution;
uniform int  uRaysPerPx;
uniform int  uMaxSteps;

uniform sampler2D uDistanceField;
uniform sampler2D uSceneMap;
uniform sampler2D uBlueNoise;
uniform sampler2D uLastFrame;

/* this shader performs "radiosity-based GI" - see comments */

vec3 raymarch(vec2 uv, vec2 dir) {
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
    // our Y coordinate is upside down
    if (uv.x != clamp(uv.x,  0.0, 1.0) || uv.y != clamp(uv.y, 0.0, 1.0))
      return vec3(0.0);

    if (dist < TAU) // surface hit#
      return max(texture2D(uLastFrame, uv).rgb, texture2D(uLastFrame, uv - (dir * (1.0/uResolution))).rgb * DECAY_RATE);
  }
  return vec3(0.0);
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
 /*
  * To calculate the light value of a pixel we cast `uRaysPerPx` amount of rays in all directions
  * and add all of the resulting samples up
  // */
  vec2 fragCoord = gl_FragCoord.xy/uResolution;

  float dist = texture(uDistanceField, fragCoord).r;
  vec3 light = texture(uLastFrame, fragCoord).rgb;

  if (dist >= TAU) { // if we're not already in a wall
    // cast rays angularly with equal angles between them
    for (float i = 0.0; i < TWO_PI; i += TWO_PI / uRaysPerPx) {
      vec3 hitcol = raymarch(fragCoord, vec2(cos(i) * uResolution.y/uResolution.x, sin(i)));
      light += hitcol;
      // brightness += max(hitcol.r, max(hitcol.g, hitcol.b));
    }
    light /= uRaysPerPx;
  }

  fragColor = vec4(light, 1.0);
}
