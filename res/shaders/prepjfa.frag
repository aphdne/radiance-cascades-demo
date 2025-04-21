#version 330 core

out vec4 fragColor;

uniform sampler2D uSceneMap;

/*
 * this shader prepares the canvas texture to be processed by the jump-flood algorithm in jfa.frag
 * all it does is replace white pixels with empty vec4s, and encodes texture coordinates into black pixels.
 */

void main() {
  vec2 fragCoord = gl_FragCoord.xy/textureSize(uSceneMap, 0); // for some reason fragTexCoord is just upside down sometimes? Raylib issue
  vec4 mask = texture(uSceneMap, fragCoord);

  if (mask.a == 1.0)
    mask = vec4(fragCoord, 0.0, 1.0);

  fragColor = mask;
}
