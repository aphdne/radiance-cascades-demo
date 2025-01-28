#version 330 core

#define PI 3.141596
#define TWO_PI 6.2831853071795864769252867665590

in vec2 fragTexCoord;

out vec4 fragColor;

uniform vec2      uResolution;
uniform int       uRaysPerPx;
uniform int       uMaxSteps;

uniform sampler2D uDistanceField;
uniform sampler2D uSceneMap;

vec3 raymarch(vec2 uv, vec2 dir) {
  float dist = 0.0;
  for (int i = 0; i < uMaxSteps; i++) {
    uv += (dir * dist)/* / (uResolution.x/uResolution.y) */;
    dist = texture(uDistanceField, uv).b;

    // if (uv/uResolution != clamp(uv/uResolution, 0.0, 1.0)) return vec3(0.0);
    // if (uv.x > 1.0 || uv.x < 0.0 || uv.y > 1.0 || uv.y < 0.0) return vec3(0.0);

    if (dist < 0.001) {
      return texture(uSceneMap, uv).rgb;
      // return max(texture(uSceneMap, uv), texture(uSceneMap, uv - (dir * (1.0/uResolution)))).rgb;
    }
  }
  return vec3(0.0);
}

void main() {
  vec2 fragCoord = gl_FragCoord.xy/uResolution;
  fragCoord.y = -fragCoord.y;

  float dist = texture(uDistanceField, fragCoord).b;
  vec3 color = texture(uSceneMap, fragCoord).rgb;

  if (dist >= 0.001) {
    float brightness = max(color.r, max(color.g, color.b));

    for (float i = 0.0; i < TWO_PI; i += TWO_PI / uRaysPerPx) {
      vec2 dir = vec2(cos(i), -sin(i));
      vec3 hitcol = raymarch(fragCoord, dir);
      color += hitcol;
      brightness += max(hitcol.r, max(hitcol.g, hitcol.b));
    }

    color = (color / brightness) * (brightness / uRaysPerPx);
  }

  fragColor = vec4(color, 1.0);
}
