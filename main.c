#include <stdio.h>
#include <stdlib.h>
#include "screensaver.h"

int main(void) {
  int n = 2;
  Body bodies[2];
  float ax[2], ay[2];
  float dt = 1.0f;

  srand(42);
  init_bodies(bodies, n);

  for (int step = 0; step < 300; step++) {
    calculate_forces(bodies, n, ax, ay);
    update_bodies(bodies, n, ax, ay, dt);
    printf("step %2d -> x=%.3f y=%.3f vx=%.3f vy=%.3f\n", step, bodies[1].x, bodies[1].y, bodies[1].vx, bodies[1].vy);
  }

  return 0;
}
