#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "screensaver.h"
#include "render.h"
#include "spatial.h"
#include "bench.h"


int parse_args(int argc, char *argv[], int *n_bodies, enum ExecMode *mode, int *schedule_kind) {
  if (argc < 2 || argc > 4) {
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

  *mode = SECUENCIAL;
  if (argc >= 3) {
    if (strcmp(argv[2], "seq") == 0 || strcmp(argv[2], "secuencial") == 0) {
      *mode = SECUENCIAL;
    } else if (strcmp(argv[2], "datos") == 0 || strcmp(argv[2], "paralelo") == 0) {
      *mode = PARALELO_DATOS;
    } else if (strcmp(argv[2], "espacial") == 0 || strcmp(argv[2], "spatial") == 0) {
      *mode = ESPACIAL;
    } else if (strcmp(argv[2], "tareas") == 0 || strcmp(argv[2], "tasks") == 0) {
      *mode = TAREAS;
    } else {
      fprintf(stderr, "Error: modo '%s' invalido (usar seq | datos | espacial | tareas)\n", argv[2]);
      return 0;
    }
  }

  *schedule_kind = 0;
  if (argc == 4) {
    if (*mode != ESPACIAL && *mode != PARALELO_DATOS) {
      // Nota: TAREAS decide su propio reparto dinamico via omp task,
      // no acepta el flag "schedule" (static/dynamic/guided).
      fprintf(stderr, "Error: 'schedule' solo aplica con modo=datos o modo=espacial\n");
      return 0;
    }
    if (strcmp(argv[3], "static") == 0) {
      *schedule_kind = 0;
    } else if (strcmp(argv[3], "dynamic") == 0) {
      *schedule_kind = 1;
    } else if (strcmp(argv[3], "guided") == 0) {
      *schedule_kind = 2;
    } else {
      fprintf(stderr, "Error: schedule '%s' invalido (usar static | dynamic | guided)\n", argv[3]);
      return 0;
    }
  }

  return 1;
}

int main(int argc, char *argv[]) {
  // Modo benchmark: no debe inicializar raylib ni abrir ventana (ver
  // seccion 9-10 del plan de optimizacion). Se atiende antes que
  // parse_args() porque usa su propio formato de flags (--bodies, --mode,
  // etc.) en vez del formato posicional del modo interactivo.
  if (argc >= 2 && strcmp(argv[1], "--benchmark") == 0) {
    return run_benchmark(argc, argv);
  }

  int n;
  enum ExecMode mode;
  int schedule_kind;
  if (!parse_args(argc, argv, &n, &mode, &schedule_kind)) {
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


  SpatialGrid grid;
  int grid_ready = 0;
  if (mode == ESPACIAL) {
    if (!spatial_grid_init(&grid, n)) {
      fprintf(stderr, "Error: no se pudo reservar el grid espacial para %d cuerpos\n", n);
      free(bodies);
      free(ax);
      free(ay);
      return 1;
    }
    grid_ready = 1;
    SpatialScheduleKind kind = SPATIAL_SCHEDULE_STATIC;
    if (schedule_kind == 1) kind = SPATIAL_SCHEDULE_DYNAMIC;
    else if (schedule_kind == 2) kind = SPATIAL_SCHEDULE_GUIDED;
    spatial_set_schedule(kind, 0);
    printf("Modo: ESTRATEGIA_2_ESPACIAL, schedule=%s\n", spatial_schedule_name());
  } else if (mode == PARALELO_DATOS) {
    set_forces_schedule(schedule_kind);
    printf("Modo: ESTRATEGIA_1_DATOS, schedule=%s\n", forces_schedule_name());
  } else if (mode == TAREAS) {
    printf("Modo: ESTRATEGIA_3_TAREAS\n");
  } else {
    printf("Modo: SECUENCIAL\n");
  }

  srand(42);
  init_bodies(bodies, n);

  float explosion_threshold = SUN_BASE_MASS + total_planet_mass(bodies, n) * EXPLOSION_ABSORPTION_FRACTION;

  RenderContext ctx;
  if (!render_init(&ctx, "N-Body Screensaver", WIDTH, HEIGHT)) {
    if (grid_ready) spatial_grid_free(&grid);
    free(bodies);
    free(ax);
    free(ay);
    return 1;
  }

  enum SimState state = STATE_RUNNING;
  int active_n = n;
  float explosion_timer = 0.0f;
  int frame_count = 0;

  while (render_poll_events(&ctx)) {
    float dt = render_get_delta_time(&ctx) * SIM_TIME_SCALE;

    if (state == STATE_RUNNING) {
      if (mode == ESPACIAL) {
        spatial_grid_build(&grid, bodies, active_n);
        calculate_forces_spatial(bodies, active_n, ax, ay, &grid);
        update_bodies(bodies, active_n, ax, ay, dt);
      } else if (mode == PARALELO_DATOS) {
        // Fuerzas + actualizacion en una sola region OpenMP (evita el
        // costo de abrir dos regiones por frame); ver simulate_step_datos().
        simulate_step_datos(bodies, active_n, ax, ay, dt);
      } else if (mode == TAREAS) {
        simulate_step_tasks(bodies, active_n, ax, ay, dt);
      } else {
        calculate_forces(bodies, active_n, ax, ay);
        update_bodies(bodies, active_n, ax, ay, dt);
      }
      active_n = check_and_merge_collisions(bodies, active_n);

      if (frame_count % 30 == 0) {
        printf("frame %d: active_n=%d sun_mass=%.1f sun_radius=%.2f fps=%.1f\n",
               frame_count, active_n, bodies[0].mass, bodies[0].radius, render_get_fps(&ctx));
      }

      if (should_explode(bodies, active_n, explosion_threshold)) {
        printf("EXPLOSION en frame %d (active_n=%d, sun_mass=%.1f)\n",
               frame_count, active_n, bodies[0].mass);
        state = STATE_EXPLODING;
        explosion_timer = EXPLOSION_DURATION_SECONDS;
      }
    } else {
      explosion_timer -= render_get_delta_time(&ctx);
      if (explosion_timer <= 0.0f) {
        init_bodies(bodies, n);
        active_n = n;
        explosion_threshold = SUN_BASE_MASS + total_planet_mass(bodies, n) * EXPLOSION_ABSORPTION_FRACTION;
        state = STATE_RUNNING;
        printf("REINICIO en frame %d\n", frame_count);
      }
    }

    render_clear(&ctx, 10, 10, 20);
    if (state == STATE_RUNNING) {
      render_sun_corona(&ctx, bodies);
      render_bodies(&ctx, bodies, active_n);
    } else {
      render_explosion_effect(&ctx, bodies, active_n, explosion_timer);
    }
    render_present(&ctx);

    render_update_fps(&ctx);
    frame_count++;
  }

  render_shutdown(&ctx);

  if (grid_ready) spatial_grid_free(&grid);
  free(bodies);
  free(ax);
  free(ay);

  return 0;
}