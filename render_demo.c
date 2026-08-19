#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "render.h"

//Programa de prueba para validar el funcionamiento del render y el contador de FPS con
//datos mock
#define N_TEST_BODIES 200

static void init_test_bodies(Body *bodies, int n) {
  for (int i = 0; i < n; i++) {
    bodies[i].x  = (float)(rand() % WIDTH);
    bodies[i].y  = (float)(rand() % HEIGHT);
    bodies[i].vx = ((float)(rand() % 200) - 100.0f) / 40.0f;
    bodies[i].vy = ((float)(rand() % 200) - 100.0f) / 40.0f;
    bodies[i].radius = 2.0f + (float)(rand() % 4);
    bodies[i].mass   = 1.0f;
    bodies[i].r = rand_color_channel();
    bodies[i].g = rand_color_channel();
    bodies[i].b = rand_color_channel();
  }
}

static void update_test_bodies(Body *bodies, int n) {
  for (int i = 0; i < n; i++) {
    bodies[i].x += bodies[i].vx;
    bodies[i].y += bodies[i].vy;

    if (bodies[i].x - bodies[i].radius < 0 || bodies[i].x + bodies[i].radius > WIDTH) {
      bodies[i].vx *= -1.0f;
    }
    if (bodies[i].y - bodies[i].radius < 0 || bodies[i].y + bodies[i].radius > HEIGHT) {
      bodies[i].vy *= -1.0f;
    }
  }
}

int main(void) {
  srand((unsigned int)time(NULL));

  RenderContext ctx;
  if (!render_init(&ctx, "Render Demo - datos de prueba", WIDTH, HEIGHT)) {
    return 1;
  }

  Body bodies[N_TEST_BODIES];
  init_test_bodies(bodies, N_TEST_BODIES);

  while (render_poll_events(&ctx)) {
    update_test_bodies(bodies, N_TEST_BODIES);

    render_clear(&ctx, 10, 10, 20);
    render_bodies(&ctx, bodies, N_TEST_BODIES);
    render_present(&ctx);

    render_update_fps(&ctx);
  }

  render_shutdown(&ctx);
  return 0;
}
