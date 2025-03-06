#version 330

out vec4 fragColor;

uniform sampler2D uEditTexture;

uniform vec2 uResolution;
uniform vec4 uColor;

bool sdfCircle(vec2 pos, float r) {
  return (distance(gl_FragCoord.xy/uResolution, pos) < r);
}

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue

  fragColor = texture(uEditTexture, fragCoord);

  if (sdfCircle(fragCoord, 10)) {
    fragColor = uColor;
  }
}
