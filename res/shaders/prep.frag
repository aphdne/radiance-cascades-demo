#version 330 core

in vec2 fragTexCoord;

out vec4 fragColor;

uniform sampler2D uCanvas;

void main() {
  vec4 mask = texture(uCanvas, vec2(fragTexCoord.x, -fragTexCoord.y));
  if (mask == vec4(1.0)) {
    mask = vec4(0.0);
  } else /* if (mask == vec4(0.0, 0.0, 0.0, 1.0)) */ {
    mask = vec4(fragTexCoord.x, fragTexCoord.y, 0.0, 1.0);
  }

  fragColor = mask;
}
