#version 330

out vec4 fragColor;

uniform vec2 uResolution;
uniform vec2 uMousePos;
uniform vec2 uLastMousePos;
uniform int uMouseDown;
uniform float uBrushSize;
uniform vec4 uBrushColor;
uniform float uTime;
uniform int uRainbow;

// sourced from https://gist.github.com/983/e170a24ae8eba2cd174f
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

bool sdfCircle(vec2 pos, float r) {
  return distance(gl_FragCoord.xy, pos) < r;
}

void main() {
  vec2 mousePos = vec2(uMousePos.x, uMousePos.y);
  vec2 lastMousePos = vec2(uLastMousePos.x, uLastMousePos.y);

  if (uMouseDown == 0) return;

  if (sdfCircle(mousePos, uBrushSize*64)) {
    fragColor = vec4(uBrushColor.rgb, 1.0);
    fragColor = vec4((uRainbow == 1) ? hsv2rgb(vec3(uTime, 1.0, 1.0)) : uBrushColor.rgb, 1.0);
  }
}
