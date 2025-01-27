#version 330 core

in vec2 fragTexCoord;

out vec4 fragColor;

uniform sampler2D uCanvas;
uniform int uJumpSize;
uniform vec2 uResolution;

/* this shader performs the jump-flood algorithm */

void main() {
  /*
   * Our seed texture (uCanvas) contains pixels that either indicate no data (their alpha is 0)
   * or a texture coordinate encoded in their red-green channels (pixels that contain encoded
   * texture coordinates are referred to as 'seeds').
   * For a pixel at (x, y), we gather a maximum of nine seeds that neighbour that pixel with (x + i, y + j)
   * where i, j ∈ {-1, 0, 1} to decide which is the closest seed known so far to the pixel.
   * We also apply an offset ('jump') of smaller sizes each round (nth term = n/2) which is multiplied
   * with our neighbour's coordinate so that i, j ∈ {-1*offset, 0, 1*offset}, e.g. if we jump 64 pixels
   * i, j ∈ {-64, 0, 64}.
   * The closest seed found for each pixel has its encoded texture coordinate written into that pixel's,
   * red-green channels and its distance stored in the pixel's blue channel so that we can create a distance
   * field later on.
   */

  vec2 fragCoord = gl_FragCoord.xy/uResolution; // for some reason fragTexCoord is just upside down sometimes? Raylib issue
  float closest = 9999.0;
  for (int Nx = -1; Nx <= 1; Nx++) {
    for (int Ny = -1; Ny <= 1; Ny++) {
      vec2 NTexCoord = fragCoord + (vec2(Nx, Ny) * uJumpSize) / uResolution;
      vec4 Nsample = texture(uCanvas, NTexCoord);

      // if (NTexCoord != clamp(NTexCoord, 0.0, 1.0)) continue; // skip pixels outside frame
      // if (Nsample.a == 0) continue;                          // skip pixels with no encoded texture coordinates

      float d = length(Nsample.rg - fragCoord);
      if (d < closest) {
        closest = d;
        fragColor = vec4(Nsample.rg, d, 1.0);
      }
    }
  }
}
