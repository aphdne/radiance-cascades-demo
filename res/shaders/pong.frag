#version 330

out vec4 fragColor;

uniform vec2 uResolution;

struct Paddle {
  vec2 position;
  bool direction; // up or down
  vec3 color;
};

struct Ball {
  vec2 position;
  vec2 direction;
  int size;
  vec3 color;
};

uniform Ball uBall;
uniform Paddle uLeftPaddle;
uniform Paddle uRightPaddle;
uniform vec2 uPaddleSize;
// uniform int uRainbow; // boolean

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

bool sdfAABB(vec2 pos, vec2 size) {
  return (gl_FragCoord.x > pos.x-size.x &&
          gl_FragCoord.x < pos.x+size.x &&
          gl_FragCoord.y > pos.y-size.y &&
          gl_FragCoord.y < pos.y+size.y);
}


void main() {
  if (sdfCircle(uBall.position, uBall.size)) {
    fragColor = vec4(uBall.color, 1.0);
  } else if (sdfAABB(uLeftPaddle.position, uPaddleSize)) {
    fragColor = vec4(uLeftPaddle.color, 1.0);
  } else if (sdfAABB(uRightPaddle.position, uPaddleSize)) {
    fragColor = vec4(uRightPaddle.color, 1.0);
  } else {
    fragColor = vec4(0.0);
  }
}
