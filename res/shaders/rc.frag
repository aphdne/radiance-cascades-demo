#version 330 core

in vec2 fragTexCoord;

out vec4 fragColor;

uniform sampler2D uCanvas;
uniform vec2 uResolution;

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue
  fragCoord = vec2(fragCoord.x, -fragCoord.y);
  fragColor = texture(uCanvas, fragCoord);
  fragColor = vec4(vec3(fragColor.b), 1.0); // show distance field only
}
