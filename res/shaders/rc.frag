#version 330 core

in vec2 fragTexCoord;

out vec4 fragColor;

uniform sampler2D uCanvas;
uniform vec2 uResolution;

void main() {
  fragColor = texture(uCanvas, fragTexCoord);
  fragColor = vec4(vec3(fragColor.b), 1.0);
}
