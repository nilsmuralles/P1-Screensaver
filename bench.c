#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "screensaver.h"
#include "spatial.h"
#include "barnes_hut.h"
#include "bench.h"

#ifdef _OPENMP
#include <omp.h>
#else
#include <time.h>
#endif

typedef enum {
  BENCH_SEQ,
  BENCH_DATOS,
  BENCH_ESPACIAL,
  BENCH_NEWTON3,
  BENCH_SOA,
  BENCH_BARNES_HUT
} BenchMode;

static double now_seconds(void) {
#ifdef _OPENMP
  return omp_get_wtime();
#else
  return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

static int cmp_double(const void *a, const void *b) {
  double da = *(const double *)a;
  double db = *(const double *)b;
  return (da > db) - (da < db);
}

static double median(double *values, int n) {
  qsort(values, n, sizeof(double), cmp_double);
  if (n % 2 == 1) return values[n / 2];
  return (values[n / 2 - 1] + values[n / 2]) / 2.0;
}

static const char *bench_mode_name(BenchMode m) {
  switch (m) {
    case BENCH_SEQ:        return "secuencial";
    case BENCH_DATOS:      return "datos";
    case BENCH_ESPACIAL:   return "espacial";
    case BENCH_NEWTON3:    return "newton3";
    case BENCH_SOA:        return "soa";
    case BENCH_BARNES_HUT: return "barnes_hut";
  }
  return "?";
}

static int parse_int_arg(const char *s, int *out) {
  char *end;
  long v = strtol(s, &end, 10);
  if (end == s || *end != '\0') return 0;
  *out = (int)v;
  return 1;
}

static int parse_float_arg(const char *s, float *out) {
  char *end;
  double v = strtod(s, &end);
  if (end == s || *end != '\0') return 0;
  *out = (float)v;
  return 1;
}

int run_benchmark(int argc, char *argv[]) {
  int n_bodies = -1;
  BenchMode mode = BENCH_DATOS;
  int steps = 100;
  int warmup = 5;
  int threads = -1;       // -1 = no tocar el default de OpenMP
  int schedule_kind = 0;  // 0=static, 1=dynamic, 2=guided (solo datos/espacial)
  float theta = BH_THETA_DEFAULT;
  int repeat = 1;
  float dt = 1.0f;

  for (int i = 2; i < argc; i++) { // argv[1] == "--benchmark"
    if (strcmp(argv[i], "--bodies") == 0 && i + 1 < argc) {
      if (!parse_int_arg(argv[++i], &n_bodies)) { fprintf(stderr, "Error: --bodies invalido\n"); return 1; }
    } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
      i++;
      if      (strcmp(argv[i], "seq") == 0 || strcmp(argv[i], "secuencial") == 0) mode = BENCH_SEQ;
      else if (strcmp(argv[i], "datos") == 0)                                     mode = BENCH_DATOS;
      else if (strcmp(argv[i], "espacial") == 0 || strcmp(argv[i], "spatial") == 0) mode = BENCH_ESPACIAL;
      else if (strcmp(argv[i], "newton3") == 0)                                   mode = BENCH_NEWTON3;
      else if (strcmp(argv[i], "soa") == 0)                                       mode = BENCH_SOA;
      else if (strcmp(argv[i], "barneshut") == 0 || strcmp(argv[i], "bh") == 0)    mode = BENCH_BARNES_HUT;
      else { fprintf(stderr, "Error: --mode '%s' invalido\n", argv[i]); return 1; }
    } else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) {
      if (!parse_int_arg(argv[++i], &steps)) { fprintf(stderr, "Error: --steps invalido\n"); return 1; }
    } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
      if (!parse_int_arg(argv[++i], &warmup)) { fprintf(stderr, "Error: --warmup invalido\n"); return 1; }
    } else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
      if (!parse_int_arg(argv[++i], &threads)) { fprintf(stderr, "Error: --threads invalido\n"); return 1; }
    } else if (strcmp(argv[i], "--schedule") == 0 && i + 1 < argc) {
      i++;
      if      (strcmp(argv[i], "static") == 0)  schedule_kind = 0;
      else if (strcmp(argv[i], "dynamic") == 0) schedule_kind = 1;
      else if (strcmp(argv[i], "guided") == 0)  schedule_kind = 2;
      else { fprintf(stderr, "Error: --schedule '%s' invalido\n", argv[i]); return 1; }
    } else if (strcmp(argv[i], "--theta") == 0 && i + 1 < argc) {
      if (!parse_float_arg(argv[++i], &theta)) { fprintf(stderr, "Error: --theta invalido\n"); return 1; }
    } else if (strcmp(argv[i], "--repeat") == 0 && i + 1 < argc) {
      if (!parse_int_arg(argv[++i], &repeat)) { fprintf(stderr, "Error: --repeat invalido\n"); return 1; }
    } else if (strcmp(argv[i], "--dt") == 0 && i + 1 < argc) {
      if (!parse_float_arg(argv[++i], &dt)) { fprintf(stderr, "Error: --dt invalido\n"); return 1; }
    } else {
      fprintf(stderr, "Error: argumento desconocido '%s'\n", argv[i]);
      return 1;
    }
  }

  if (n_bodies < 2 || n_bodies > MAX_BODIES) {
    fprintf(stderr, "Error: --bodies es obligatorio y debe estar en [2, %d]\n", MAX_BODIES);
    return 1;
  }
  if (steps < 1 || warmup < 0 || repeat < 1) {
    fprintf(stderr, "Error: --steps/--warmup/--repeat fuera de rango\n");
    return 1;
  }

  int effective_threads;
#ifdef _OPENMP
  if (threads > 0) omp_set_num_threads(threads);
  effective_threads = omp_get_max_threads();
#else
  effective_threads = 1;
  if (threads > 1) {
    fprintf(stderr, "Aviso: binario compilado sin OpenMP (usa 'make omp'); --threads se ignora.\n");
  }
#endif

  if (mode == BENCH_DATOS) {
    set_forces_schedule(schedule_kind);
  }
  if (mode == BENCH_ESPACIAL) {
    SpatialScheduleKind kind = SPATIAL_SCHEDULE_STATIC;
    if (schedule_kind == 1) kind = SPATIAL_SCHEDULE_DYNAMIC;
    else if (schedule_kind == 2) kind = SPATIAL_SCHEDULE_GUIDED;
    spatial_set_schedule(kind, 0);
  }

  Body *bodies = malloc((size_t)n_bodies * sizeof(Body));
  float *ax = malloc((size_t)n_bodies * sizeof(float));
  float *ay = malloc((size_t)n_bodies * sizeof(float));
  if (bodies == NULL || ax == NULL || ay == NULL) {
    fprintf(stderr, "Error: no se pudo reservar memoria para %d cuerpos\n", n_bodies);
    free(bodies); free(ax); free(ay);
    return 1;
  }

  SpatialGrid grid;
  int grid_ready = 0;
  if (mode == BENCH_ESPACIAL) {
    if (!spatial_grid_init(&grid, n_bodies)) {
      fprintf(stderr, "Error: no se pudo reservar el grid espacial\n");
      free(bodies); free(ax); free(ay);
      return 1;
    }
    grid_ready = 1;
  }

  BHTree tree;
  int tree_ready = 0;
  if (mode == BENCH_BARNES_HUT) {
    if (!bh_tree_init(&tree, 8 * n_bodies + 16)) {
      fprintf(stderr, "Error: no se pudo reservar el arbol Barnes-Hut\n");
      if (grid_ready) spatial_grid_free(&grid);
      free(bodies); free(ax); free(ay);
      return 1;
    }
    tree_ready = 1;
  }

  PhysicsSoA soa;
  int soa_ready = 0;
  if (mode == BENCH_SOA) {
    if (!physics_soa_init(&soa, n_bodies)) {
      fprintf(stderr, "Error: no se pudo reservar la SoA\n");
      if (grid_ready) spatial_grid_free(&grid);
      if (tree_ready) bh_tree_free(&tree);
      free(bodies); free(ax); free(ay);
      return 1;
    }
    soa_ready = 1;
  }

  // Cada repeticion rehace init_bodies() con la misma semilla, asi que
  // todas parten exactamente del mismo estado inicial.
  double *kernel_times = malloc((size_t)repeat * sizeof(double));
  double *total_times  = malloc((size_t)repeat * sizeof(double));

  for (int rep = 0; rep < repeat; rep++) {
    srand(42);
    init_bodies(bodies, n_bodies);

    double kernel_accum = 0.0;
    double total_accum = 0.0;

    for (int step = 0; step < warmup + steps; step++) {
      int measured = (step >= warmup);

      double t_kernel_start = measured ? now_seconds() : 0.0;
      switch (mode) {
        case BENCH_SEQ:
          calculate_forces(bodies, n_bodies, ax, ay);
          break;
        case BENCH_DATOS:
          calculate_forces_parallel(bodies, n_bodies, ax, ay);
          break;
        case BENCH_ESPACIAL:
          spatial_grid_build(&grid, bodies, n_bodies);
          calculate_forces_spatial(bodies, n_bodies, ax, ay, &grid);
          break;
        case BENCH_NEWTON3:
          calculate_forces_parallel_newton3(bodies, n_bodies, ax, ay);
          break;
        case BENCH_SOA:
          physics_soa_sync(&soa, bodies, n_bodies);
          calculate_forces_soa_parallel(&soa, n_bodies, ax, ay);
          break;
        case BENCH_BARNES_HUT:
          bh_tree_build(&tree, bodies, n_bodies);
          calculate_forces_barnes_hut(&tree, bodies, n_bodies, ax, ay, theta);
          break;
      }
      double t_kernel_end = measured ? now_seconds() : 0.0;

      double t_update_start = measured ? now_seconds() : 0.0;
      update_bodies(bodies, n_bodies, ax, ay, dt);
      double t_update_end = measured ? now_seconds() : 0.0;

      if (measured) {
        kernel_accum += (t_kernel_end - t_kernel_start);
        total_accum  += (t_kernel_end - t_kernel_start) + (t_update_end - t_update_start);
      }
      // Fusiones y explosiones quedan deliberadamente desactivadas durante
      // la medicion: N y la carga por paso se mantienen constantes.
    }

    kernel_times[rep] = kernel_accum;
    total_times[rep] = total_accum;
  }

  double kernel_med = median(kernel_times, repeat);
  double total_med = median(total_times, repeat);

  printf("=== Benchmark headless ===\n");
  printf("modo=%s N=%d pasos=%d warmup=%d repeticiones=%d hilos=%d dt=%.4f\n",
         bench_mode_name(mode), n_bodies, steps, warmup, repeat, effective_threads, dt);
  if (mode == BENCH_DATOS)      printf("schedule=%s\n", forces_schedule_name());
  if (mode == BENCH_ESPACIAL)   printf("schedule=%s\n", spatial_schedule_name());
  if (mode == BENCH_BARNES_HUT) printf("theta=%.3f\n", theta);
  printf("kernel:        mediana=%.6f s  (%.6f s/paso, %.1f pasos/s)\n",
         kernel_med, kernel_med / steps, steps / kernel_med);
  printf("fisica_total:  mediana=%.6f s  (%.6f s/paso, %.1f pasos/s)  [kernel + update_bodies]\n",
         total_med, total_med / steps, steps / total_med);

  free(kernel_times);
  free(total_times);
  if (grid_ready) spatial_grid_free(&grid);
  if (tree_ready) bh_tree_free(&tree);
  if (soa_ready) physics_soa_free(&soa);
  free(bodies);
  free(ax);
  free(ay);

  return 0;
}