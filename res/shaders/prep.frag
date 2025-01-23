#version 330 core

out vec4 fragColor;

uniform sampler2D uCanvas;
uniform vec2 uResolution;

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue
  vec4 mask = texture(uCanvas, fragCoord);
  if (mask == vec4(1.0)) {
    mask = vec4(0.0);
  } else /* if (mask == vec4(0.0, 0.0, 0.0, 1.0)) */ {
    mask = vec4(fragCoord, 0.0, 1.0);
  }

  fragColor = mask;
}
