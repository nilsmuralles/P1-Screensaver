#include <stdlib.h>
#include <math.h>
#include "spatial.h"

#ifdef _OPENMP
#include <omp.h>
#endif

static int clamp_int(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static int cell_index_of(const Body *b, float cell_width, float cell_height) {
  int col = (int)(b->x / cell_width);
  int row = (int)(b->y / cell_height);
  col = clamp_int(col, 0, SPATIAL_GRID_COLS - 1);
  row = clamp_int(row, 0, SPATIAL_GRID_ROWS - 1);
  return row * SPATIAL_GRID_COLS + col;
}

int spatial_grid_init(SpatialGrid *grid, int max_bodies) {
  grid->cell_bodies = malloc((size_t)max_bodies * sizeof(int));
  if (grid->cell_bodies == NULL) {
    return 0;
  }
  grid->capacity = max_bodies;
  grid->cell_width  = (float)WIDTH  / (float)SPATIAL_GRID_COLS;
  grid->cell_height = (float)HEIGHT / (float)SPATIAL_GRID_ROWS;
  for (int c = 0; c <= SPATIAL_GRID_CELLS; c++) {
    grid->cell_start[c] = 0;
  }
  return 1;
}

void spatial_grid_free(SpatialGrid *grid) {
  free(grid->cell_bodies);
  grid->cell_bodies = NULL;
  grid->capacity = 0;
}

void spatial_grid_build(SpatialGrid *grid, const Body *bodies, int n_bodies) {
  int counts[SPATIAL_GRID_CELLS] = {0};

  for (int i = 0; i < n_bodies; i++) {
    int c = cell_index_of(&bodies[i], grid->cell_width, grid->cell_height);
    counts[c]++;
  }

  grid->cell_start[0] = 0;
  for (int c = 0; c < SPATIAL_GRID_CELLS; c++) {
    grid->cell_start[c + 1] = grid->cell_start[c] + counts[c];
  }

  int cursor[SPATIAL_GRID_CELLS];
  for (int c = 0; c < SPATIAL_GRID_CELLS; c++) {
    cursor[c] = grid->cell_start[c];
  }

  for (int i = 0; i < n_bodies; i++) {
    int c = cell_index_of(&bodies[i], grid->cell_width, grid->cell_height);
    grid->cell_bodies[cursor[c]] = i;
    cursor[c]++;
  }
}

#ifdef _OPENMP
static omp_sched_t g_schedule_kind = omp_sched_static;
static int g_schedule_chunk = 0;
#endif

void spatial_set_schedule(SpatialScheduleKind kind, int chunk_size) {
#ifdef _OPENMP
  switch (kind) {
    case SPATIAL_SCHEDULE_DYNAMIC: g_schedule_kind = omp_sched_dynamic; break;
    case SPATIAL_SCHEDULE_GUIDED:  g_schedule_kind = omp_sched_guided;  break;
    case SPATIAL_SCHEDULE_STATIC:
    default:                       g_schedule_kind = omp_sched_static; break;
  }
  g_schedule_chunk = chunk_size;
  omp_set_schedule(g_schedule_kind, g_schedule_chunk);
#else
  (void)kind;
  (void)chunk_size;
#endif
}

const char *spatial_schedule_name(void) {
#ifdef _OPENMP
  switch (g_schedule_kind) {
    case omp_sched_dynamic: return "dynamic";
    case omp_sched_guided:  return "guided";
    default:                return "static";
  }
#else
  return "secuencial (sin OpenMP)";
#endif
}


static void accumulate_force(const Body *bodies, int n_bodies, int i, float *out_ax, float *out_ay) {
  float sum_ax = 0.0f;
  float sum_ay = 0.0f;

  for (int j = 0; j < n_bodies; j++) {
    if (i == j) continue;

    float dx = bodies[j].x - bodies[i].x;
    float dy = bodies[j].y - bodies[i].y;
    float r2 = dx * dx + dy * dy + EPSILON * EPSILON;
    float r  = sqrtf(r2);

    float f = CONST_G * bodies[j].mass / (r2 * r);

    sum_ax += f * dx;
    sum_ay += f * dy;
  }

  *out_ax = sum_ax;
  *out_ay = sum_ay;
}

void calculate_forces_spatial(const Body *bodies, int n_bodies, float *ax, float *ay, const SpatialGrid *grid) {
#ifdef _OPENMP
  #pragma omp parallel for schedule(runtime)
  for (int c = 0; c < SPATIAL_GRID_CELLS; c++) {
    int start = grid->cell_start[c];
    int end   = grid->cell_start[c + 1];
    for (int k = start; k < end; k++) {
      int i = grid->cell_bodies[k];
      accumulate_force(bodies, n_bodies, i, &ax[i], &ay[i]);
    }
  }
#else
  for (int c = 0; c < SPATIAL_GRID_CELLS; c++) {
    int start = grid->cell_start[c];
    int end   = grid->cell_start[c + 1];
    for (int k = start; k < end; k++) {
      int i = grid->cell_bodies[k];
      accumulate_force(bodies, n_bodies, i, &ax[i], &ay[i]);
    }
  }
#endif
}