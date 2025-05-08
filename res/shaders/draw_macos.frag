#version 330

out vec4 fragColor;

uniform sampler2D uCanvas;

uniform vec2 uMousePos;
uniform vec2 uLastMousePos;
uniform int uMouseDown;
uniform float uBrushSize;
uniform vec4 uBrushColor;
uniform float uTime;
uniform int uRainbow; // boolean

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
  bool sdf = false;
  #define LERP_AMOUNT 16.0
  for (float i = 0; i < LERP_AMOUNT; i++) {
    if (sdfCircle(mix(uMousePos, uLastMousePos, i/(LERP_AMOUNT)), uBrushSize*64))
      sdf = true;
  }

  if (sdf && uMouseDown == 1)
    fragColor = vec4((uRainbow == 1) ? hsv2rgb(vec3(uTime, 1.0, 1.0)) : uBrushColor.rgb, 1.0);
  else
    fragColor = texture(uCanvas, gl_FragCoord.xy/textureSize(uCanvas, 0));
}
