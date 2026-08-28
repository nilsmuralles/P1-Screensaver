#include <stdio.h>
#include <stdlib.h>
#include "screensaver.h"
#include "render.h"

int parse_args(int argc, char *argv[], int *n_bodies) {
  if (argc != 2) {
    fprintf(stderr, "Uso: %s <N>\n", argv[0]);
    fprintf(stderr, "  N: cantidad de cuerpos a simular (incluye el sol, minimo 2)\n");
    return 0;
  }

  char *endptr;
  long n = strtol(argv[1], &endptr, 10);

  if (endptr == argv[1] || *endptr != '\0') {
    fprintf(stderr, "Error: '%s' no es un numero entero valido\n", argv[1]);
    return 0;
  }

  if (n < 2) {
    fprintf(stderr, "Error: N debe ser al menos 2 (el sol + 1 cuerpo)\n");
    return 0;
  }

  if (n > MAX_BODIES) {
    fprintf(stderr, "Error: N no puede ser mayor a %d\n", MAX_BODIES);
    return 0;
  }

  *n_bodies = (int)n;
  return 1;
}

int main(int argc, char *argv[]) {
  int n;
  if (!parse_args(argc, argv, &n)) {
    return 1;
  }

  Body *bodies = malloc(n * sizeof(Body));
  float *ax = malloc(n * sizeof(float));
  float *ay = malloc(n * sizeof(float));

  if (bodies == NULL || ax == NULL || ay == NULL) {
    fprintf(stderr, "Error: no se pudo reservar memoria para %d cuerpos\n", n);
    free(bodies);
    free(ax);
    free(ay);
    return 1;
  }

  float dt = 1.0f;

  srand(42);
  init_bodies(bodies, n);

  float explosion_threshold = SUN_BASE_MASS + total_planet_mass(bodies, n) * EXPLOSION_ABSORPTION_FRACTION;

  RenderContext ctx;
  if (!render_init(&ctx, "N-Body Screensaver", WIDTH, HEIGHT)) {
    free(bodies);
    free(ax);
    free(ay);
    return 1;
  }

  enum SimState state = STATE_RUNNING;
  int active_n = n;
  int explosion_timer = 0;
  int frame_count = 0;

  while (render_poll_events(&ctx)) {
    if (state == STATE_RUNNING) {
      calculate_forces(bodies, active_n, ax, ay);
      update_bodies(bodies, active_n, ax, ay, dt);
      active_n = check_and_merge_collisions(bodies, active_n);

      if (frame_count % 30 == 0) {
        printf("frame %d: active_n=%d sun_mass=%.1f sun_radius=%.2f\n",
               frame_count, active_n, bodies[0].mass, bodies[0].radius);
      }

      if (should_explode(bodies, active_n, explosion_threshold)) {
        printf("EXPLOSION en frame %d (active_n=%d, sun_mass=%.1f)\n",
               frame_count, active_n, bodies[0].mass);
        state = STATE_EXPLODING;
        explosion_timer = EXPLOSION_DURATION_FRAMES;
      }
    } else { 
      explosion_timer--;
      if (explosion_timer <= 0) {
        init_bodies(bodies, n);
        active_n = n;
        explosion_threshold = SUN_BASE_MASS + total_planet_mass(bodies, n) * EXPLOSION_ABSORPTION_FRACTION;
        state = STATE_RUNNING;
        printf("REINICIO en frame %d\n", frame_count);
      }
    }

    render_clear(&ctx, 10, 10, 20);
    if (state == STATE_RUNNING) {
      render_bodies(&ctx, bodies, active_n);
    } else {
      render_explosion_effect(&ctx, bodies, active_n, explosion_timer);
    }
    render_present(&ctx);

    render_update_fps(&ctx);
    frame_count++;
  }

  render_shutdown(&ctx);

  free(bodies);
  free(ax);
  free(ay);

  return 0;
}
