#version 330 core

in vec2 fragTexCoord;

out vec4 fragColor;

uniform vec2 uResolution;

uniform sampler2D uDistanceField;
uniform sampler2D uSceneMap;

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue
  fragCoord = vec2(fragCoord.x, -fragCoord.y);
  fragColor = texture(uSceneMap, fragCoord);
  fragColor = vec4(vec3(fragColor.b), 1.0); // show distance field only
}
