#version 330 core

out vec4 fragColor;

uniform vec2      uResolution;
uniform sampler2D uSceneMap;
uniform sampler2D uLastFrame;
uniform float uMixFactor;
uniform int uFlipY;

/*
 * this shader prepares the last frame for being used in the current frame by mixing it with the scene map
 */

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution;
	fragColor = vec4(mix(texture(uSceneMap,  fragCoord).rgb, texture(uLastFrame, vec2(fragCoord.x, (uFlipY == 1) ? -fragCoord.y : fragCoord.y)).rgb, uMixFactor), 1.0);
}
