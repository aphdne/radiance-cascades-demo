#version 330 core

out vec4 fragColor;

uniform sampler2D uCanvas;
uniform int uJumpSize;
uniform vec2 uResolution;

/* this shader performs the jump-flood algorithm */

void main() {
  /*
   * for a pixel at (x, y), we gather a maximum of nine seeds stored with pixels (x + i, y + j)
   * where i, j ∈ {-1, 0, 1} to decide which is the closest seed known so far to the pixel.
   * the closest seed found for each pixel is written into the texture for the next round.
   */

  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue
  float closest = 9999.0;
  for (int Nx = -1; Nx <= 1; Nx++) {
    for (int Ny = -1; Ny <= 1; Ny++) {
      vec2 NTexCoord = fragCoord + (vec2(Nx, Ny) * uJumpSize) / uResolution;
      vec4 Nsample = texture(uCanvas, NTexCoord);

      if (NTexCoord != clamp(NTexCoord, 0.0, 1.0)) continue; // skip pixels outside frame
      if (Nsample.a == 0) continue;                          // skip pixels with no encoded texture coordinates

      float d = length(Nsample.rg - fragCoord);
      if (d < closest) {
        closest = d;
        fragColor = vec4(Nsample.rg, d, 1.0);
      }
    }
  }
}
