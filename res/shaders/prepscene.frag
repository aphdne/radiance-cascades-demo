#version 330 core

#define CENTRE vec2(resolution.x, resolution.y)/2
#define ORB_SPEED 2.0
#define ORB_SIZE resolution.x/80

out vec4 fragColor;

uniform sampler2D uOcclusionMap;
uniform sampler2D uEmissionMap;

uniform vec2 uMousePos;
uniform float uTime;
uniform int uOrbs;
uniform int uRainbow;
uniform int uMouseLight;
uniform float uBrushSize;
uniform vec4 uBrushColor;

/*
 * this shader prepares the canvas texture to be processed by the jump-flood algorithm in jfa.frag
 * all it does is replace white pixels with empty vec4s, and encodes texture coordinates into black pixels.
 */

// https://gist.github.com/983/e170a24ae8eba2cd174f
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

bool sdfCircle(vec2 pos, float r) {
  return (distance(gl_FragCoord.xy, pos) < r);
}

void main() {
  vec2 resolution = textureSize(uOcclusionMap, 0);
  vec2 fragCoord = gl_FragCoord.xy/resolution;

  vec4 o  = texture(uOcclusionMap, fragCoord);
  vec4 e  = texture(uEmissionMap, fragCoord);

  if (o == vec4(1.0))
    o = vec4(0.0);
  else
    o = vec4(0.0, 0.0, 0.0, 1.0);

  vec3 ehsv = rgb2hsv(vec3(e.rgb));
  if (e == vec4(0.0, 0.0, 0.0, 1.0))
    e = vec4(0.0);
  else if (uRainbow == 1)
    e = vec4(hsv2rgb(vec3(ehsv.r + uTime/8, 1.0, 1.0)), 1.0);

  fragColor = (max(e.a, o.a) == e.a) ? e : o;

  vec2 p;

  if (uMouseLight == 1)
    if (sdfCircle(uMousePos, uBrushSize*64))
      fragColor = uBrushColor;

  if (uOrbs == 1) {
   for (int i = 0; i < 6; i++) {
      p = (vec2(cos(uTime/ORB_SPEED+i), sin(uTime/ORB_SPEED+i)) * resolution.y/2 + 1) / 2 + CENTRE;
      if (sdfCircle(p, resolution.x/80))
        fragColor = vec4(hsv2rgb(vec3(i/6.0, 1.0, 1.0)), 1.0);
    }

    p = vec2(cos(uTime/ORB_SPEED*4) * resolution.x/4, 0) + CENTRE;
    if (sdfCircle(p, ORB_SIZE))
      fragColor = vec4(vec3(sin(uTime) + 1 / 2), 1.0);

    p = vec2(0, sin(uTime/ORB_SPEED*4) * resolution.x/4) + CENTRE;
    if (sdfCircle(p, ORB_SIZE))
      fragColor = vec4(vec3(cos(uTime) + 1 / 2), 1.0);
  }
}
