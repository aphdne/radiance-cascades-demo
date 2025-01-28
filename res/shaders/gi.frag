#version 330 core

in vec2 fragTexCoord;

out vec4 fragColor;

uniform vec2      uResolution;
uniform sampler2D uDistanceField;
uniform int       uRaysPerPx;
uniform int       uMaxSteps;

uniform sampler2D uOcclusionMap;
uniform sampler2D uEmitterMap;

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue
  fragCoord = vec2(fragCoord.x, -fragCoord.y);

  // vec4 distfield = vec4(vec3(texture(uDistanceField, fragCoord).b), 1.0);
  // fragColor = distfield;

  fragColor = texture(uEmitterMap, fragCoord);
  // fragColor = texture(uOcclusionMap, fragCoord);
  // fragColor = texture(uDistanceField, fragCoord);
}
