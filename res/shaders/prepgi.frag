#version 330 core

#define MIX_FACTOR 0.7

out vec4 fragColor;

uniform vec2      uResolution;
uniform sampler2D uSceneMap;
uniform sampler2D uLastFrame;

/*
 * this shader prepares the last frame for being used in the current frame by mixing it with the scene map
 */

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution;
	fragColor = vec4(mix(texture2D(uSceneMap,  fragCoord).rgb, texture2D(uLastFrame, vec2(fragCoord.x, -fragCoord.y)).rgb, MIX_FACTOR), 1.0);
}
