#include <stdlib.h>
#include <math.h>
#include "corona.h"

#ifdef _OPENMP
#include <omp.h>
#endif

int corona_init(CoronaBuffer *corona) {
  corona->pixels = malloc((size_t)CORONA_TEX_SIZE * CORONA_TEX_SIZE * 4);
  return corona->pixels != NULL;
}

void corona_free(CoronaBuffer *corona) {
  free(corona->pixels);
  corona->pixels = NULL;
}

static float noise2(float x, float y) {
  float h = sinf(x * 127.1f + y * 311.7f) * 43758.5453f;
  return h - floorf(h);
}

void corona_generate(CoronaBuffer *corona, float time_seconds) {
  const float center = CORONA_TEX_SIZE / 2.0f;

  #pragma omp parallel for schedule(static)
  for (int y = 0; y < CORONA_TEX_SIZE; y++) {
    for (int x = 0; x < CORONA_TEX_SIZE; x++) {
      float dx = (x - center) / center;
      float dy = (y - center) / center;
      float dist = sqrtf(dx * dx + dy * dy);

      float angle = atan2f(dy, dx);
      float flicker = 0.5f + 0.5f * sinf(angle * 6.0f + time_seconds * 1.5f
                                          + noise2((float)x, (float)y) * 3.0f);

      float falloff = 1.0f - dist;
      if (falloff < 0.0f) falloff = 0.0f;
      falloff = falloff * falloff;

      float intensity = falloff * (0.6f + 0.4f * flicker);
      if (intensity > 1.0f) intensity = 1.0f;

      int idx = (y * CORONA_TEX_SIZE + x) * 4;
      corona->pixels[idx + 0] = 255;
      corona->pixels[idx + 1] = (unsigned char)(200.0f + 55.0f * intensity);
      corona->pixels[idx + 2] = (unsigned char)(70.0f + 50.0f * intensity);
      corona->pixels[idx + 3] = (unsigned char)(255.0f * intensity);
    }
  }
}
