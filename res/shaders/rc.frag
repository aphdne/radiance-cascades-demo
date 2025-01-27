#version 330 core

in vec2 fragTexCoord;

out vec4 fragColor;

uniform sampler2D uCanvas;
uniform vec2 uResolution;
uniform vec2 uDPI;

void main() {
  vec2 fragCoord = gl_FragCoord.xy/(uResolution*uDPI);
  fragCoord = vec2(fragCoord.x, -fragCoord.y);
  fragColor = texture(uCanvas, fragCoord);
  fragColor = vec4(vec3(fragColor.b), 1.0);
}
