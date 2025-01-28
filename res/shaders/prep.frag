#version 330 core

out vec4 fragColor;

uniform sampler2D uOcclusionMap;
uniform sampler2D uEmitterMap;
uniform vec2 uResolution;

/*
 * this shader prepares the canvas texture to be processed by the jump-flood algorithm in jfa.frag
 * all it does is replace white pixels with empty vec4s, and encodes texture coordinates into black pixels.
 */

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue
  vec4 mask = texture(uOcclusionMap, fragCoord);
  if (mask == vec4(1.0)) {
    // vec4 mask2 = texture(uOcclusionMap, fragCoord);
    // if (mask2 == vec4(0.0)) {
      mask = vec4(0.0);
    // }
  } else /* if (mask == vec4(0.0, 0.0, 0.0, 1.0)) */ {
    mask = vec4(fragCoord, 0.0, 1.0);
  }

  fragColor = mask;
}
