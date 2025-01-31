#version 330 core

out vec4 fragColor;

uniform sampler2D uJFA;
uniform vec2 uResolution;

/*
 * this shader reads out the distance field contained within the JFA output.
 */

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue
  vec4 mask = texture(uJFA, fragCoord);
  fragColor = vec4(vec3(mask.b), 1.0);
}
