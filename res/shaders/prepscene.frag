#version 330 core

out vec4 fragColor;

uniform sampler2D uOcclusionMap;
uniform sampler2D uEmissionMap;
uniform sampler2D uEmissionMapMask;

uniform vec2 uResolution;
uniform float uTime;

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

bool sdfCircle(vec2 pos, float r) {
  if (distance(gl_FragCoord.xy/uResolution, pos) < r) {
    return true;
  }
  return false;
}

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue

  vec4 o  = texture(uOcclusionMap, fragCoord);
  vec4 e  = texture(uEmissionMap, fragCoord);
  vec4 em = texture(uEmissionMapMask, fragCoord);

  if (o == vec4(1.0))
    o = vec4(0.0);
  else
    o = vec4(0.0, 0.0, 0.0, 1.0);

  float v = rgb2hsv(e.xyz).z;
  if (v < 0.99)
    e = vec4(0.0);

  fragColor = (max(e.a, o.a) == e.a) ? e : o;

  vec2 p = (vec2(cos(uTime), sin(uTime)) * 0.5 + 1) / 2;
  if (sdfCircle(p, 0.03))
    fragColor = vec4(1.0);
}
