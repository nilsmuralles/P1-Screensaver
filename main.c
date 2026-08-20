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

  RenderContext ctx;
  if (!render_init(&ctx, "N-Body Screensaver", WIDTH, HEIGHT)) {
    free(bodies);
    free(ax);
    free(ay);
    return 1;
  }

  while (render_poll_events(&ctx)) {
    calculate_forces(bodies, n, ax, ay);
    update_bodies(bodies, n, ax, ay, dt);

    render_clear(&ctx, 10, 10, 20);
    render_bodies(&ctx, bodies, n);
    render_present(&ctx);

    render_update_fps(&ctx);
  }

  render_shutdown(&ctx);

  free(bodies);
  free(ax);
  free(ay);

  return 0;
}
