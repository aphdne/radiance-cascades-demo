#version 330 core

out vec4 fragColor;

uniform sampler2D uOcclusionMap;
uniform sampler2D uEmissionMap;

uniform vec2 uResolution;
uniform float uTime;

/*
 * this shader prepares the canvas texture to be processed by the jump-flood algorithm in jfa.frag
 * all it does is replace white pixels with empty vec4s, and encodes texture coordinates into black pixels.
 */

bool sdfCircle(vec2 pos, float r) {
  if (distance(gl_FragCoord.xy/uResolution, pos) < r) {
    return true;
  }
  return false;
}

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue

  vec4 o = texture(uOcclusionMap, fragCoord);
  vec4 e = texture(uEmissionMap, fragCoord);

  if (o == vec4(1.0))
    o = vec4(0.0);
  else
    o = vec4(0.0, 0.0, 0.0, 1.0);

  if (e == vec4(vec3(0.0), 1.0))
    e = vec4(0.0);
  else
    e = vec4(1.0, 0.0, 0.0, 1.0);

  fragColor = (max(e.a, o.a) == e.a) ? e : o;

  vec2 p = (vec2(cos(uTime), sin(uTime)) * 0.5 + 1) / 2;
  if (sdfCircle(p, 0.03))
    fragColor = vec4(1.0);
}
