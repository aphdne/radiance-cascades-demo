#version 330 core

out vec4 fragColor;

uniform sampler2D uJFA;
uniform vec2 uResolution;

/*
 * this shader reads out the distance field contained within the JFA output.
 * Useful as to reducing amount of data sent to RC shader
 */

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue
  fragColor = vec4(vec3(texture(uJFA, fragCoord).b), 1.0);
}
