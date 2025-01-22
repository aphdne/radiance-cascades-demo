#version 330 core

in vec2 fragTexCoord;

out vec4 fragColor;

uniform sampler2D uCanvas;
uniform int uJump;
uniform int uFirstJump;
uniform vec2 uResolution;

//Center RG value
#define CENTER 127.0/255.0
//RG value range
#define RANGE  255.0

void main() {
  fragColor = vec4(0.0);
  // fragColor = vec4(0.0);

  float bestdist = 9999.0;
  for (int Nx = -1; Nx <= 1; Nx++) {
    for (int Ny = -1; Ny <= 1; Ny++) {
      vec2 off = (vec2(Nx, Ny) * uJump) / uResolution;
      vec2 Ncoord = fragTexCoord + off;
      if (Ncoord != clamp(Ncoord, 0.0, 1.0)) continue; // skip outside texture coordinates

      vec4 Ntexture = texture(uCanvas, Ncoord);

      if (Nx == 0 && Ny == 0 && Ntexture.a == 1.0) {
        fragColor = Ntexture;
        return;
      }

      // vec2 tex_off = (Ntexture.xy - CENTER) * vec2(Ntexture.a < 1.0) + off;
      // float tex_dist = length(tex_off);
      //
      // if (dist > tex_dist && (uFirstJump == 0 || Ntexture.a >= 1.0)) {
      //   fragColor.xy = tex_off + CENTER;
      //   dist = tex_dist;
      //   fragColor.a = 1.0 - tex_dist * 3.0;
      // }

      float d = length(Ntexture.xy - fragTexCoord);
      if (bestdist < d && uFirstJump == 0) {
        bestdist = d;
        fragColor = vec4(Ntexture.xy, 0.0, 1.0);
      }
    }
  }
}
