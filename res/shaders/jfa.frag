#version 330 core

#define CENTER 127.0/255.0

in vec2 fragTexCoord;

out vec4 fragColor;

uniform sampler2D uCanvas;
uniform int uJumpSize;
uniform vec2 uResolution;

/* this shader performs the Jump-Flood algorithm (JFA) */

void main() {
  /*
   * Our 'seed texture' from prep.frag (uCanvas) contains pixels that either indicate no data (their alpha is 0)
   * or a texture coordinate encoded in their red-green channels (pixels that encode texture coordinates are referred to as 'seeds').
   * For a pixel at (x, y) we gather (a maximum of) eight seeds that neighbour that pixel as (x + i, y + j) where i, j ∈ {-1, 0, 1}
   * to decide which is the closest seed known so far to that pixel.
   *
   * We also apply an offset ('jump') of smaller sizes each round (n/2, n/4, n/8 ... 1) which is multiplied with our neighbour's coordinate
   * so that i, j ∈ {-1*offset, 0, 1*offset}, e.g. if we jump 64 pixels i, j ∈ {-64, 0, 64}.
   *
   * The closest seed found for each pixel has its encoded texture coordinate written into that pixel's red-green channels and its distance
   * stored in the pixel's blue channel so that we can create a distance field later on for radiance cascading.
   *
   * This essentially means that after running this shader the input texture will be processed so that each previously-empty pixel contains
   * the coordinates to its nearest seed.
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
