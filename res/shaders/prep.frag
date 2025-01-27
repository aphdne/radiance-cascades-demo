#version 330 core

out vec4 fragColor;

uniform sampler2D uCanvas;
uniform vec2 uResolution;
uniform vec2 uDPI;

/* this shader prepares the canvas texture to be processed by the jump-flood algorithm jfa.frag */

void main() {
  // vec2 dpi = vec2(1/pow(uDPI.x, 2), 1/pow(uDPI.y, 2));
  vec2 fragCoord = gl_FragCoord.xy/(uResolution); // for some reason fragTexCoord is just upside down sometimes? Raylib issue
  vec4 mask = texture(uCanvas, fragCoord);
  if (mask == vec4(1.0)) {
    mask = vec4(0.0);
  } else /* if (mask == vec4(0.0, 0.0, 0.0, 1.0)) */ {
    mask = vec4(fragCoord, 0.0, 1.0);
  }

  fragColor = mask;
  // fragColor = vec4(fragCoord, 0.0, 1.0);
}
