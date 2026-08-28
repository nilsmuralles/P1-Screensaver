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

static int cell_index_of(const SpatialGrid *grid, const Body *b) {
  int col = (int)((b->x - grid->origin_x) / grid->cell_width);
  int row = (int)((b->y - grid->origin_y) / grid->cell_height);
  col = clamp_int(col, 0, grid->grid_cols - 1);
  row = clamp_int(row, 0, grid->grid_rows - 1);
  return row * grid->grid_cols + col;
}

int spatial_grid_init(SpatialGrid *grid, int max_bodies) {
  grid->cell_bodies = malloc((size_t)max_bodies * sizeof(int));
  if (grid->cell_bodies == NULL) {
    return 0;
  }
  grid->capacity = max_bodies;

  int target_cells = max_bodies / SPATIAL_BODIES_PER_CELL_TARGET;
  if (target_cells < 1) target_cells = 1;
  int side = (int)ceilf(sqrtf((float)target_cells));
  side = clamp_int(side, 1, SPATIAL_MAX_GRID_COLS);

  grid->grid_cols = side;
  grid->grid_rows = clamp_int(side, 1, SPATIAL_MAX_GRID_ROWS);
  grid->grid_cells = grid->grid_cols * grid->grid_rows;

  grid->origin_x = 0.0f;
  grid->origin_y = 0.0f;
  grid->cell_width  = (float)WIDTH  / (float)grid->grid_cols;
  grid->cell_height = (float)HEIGHT / (float)grid->grid_rows;

  for (int c = 0; c <= grid->grid_cells; c++) {
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
  float min_x = bodies[0].x, max_x = bodies[0].x;
  float min_y = bodies[0].y, max_y = bodies[0].y;
  for (int i = 1; i < n_bodies; i++) {
    if (bodies[i].x < min_x) min_x = bodies[i].x;
    if (bodies[i].x > max_x) max_x = bodies[i].x;
    if (bodies[i].y < min_y) min_y = bodies[i].y;
    if (bodies[i].y > max_y) max_y = bodies[i].y;
  }

  const float PADDING = 4.0f;
  float span_x = (max_x - min_x) + PADDING;
  float span_y = (max_y - min_y) + PADDING;

  grid->origin_x = min_x - PADDING * 0.5f;
  grid->origin_y = min_y - PADDING * 0.5f;
  grid->cell_width  = span_x / (float)grid->grid_cols;
  grid->cell_height = span_y / (float)grid->grid_rows;

  int counts[SPATIAL_MAX_GRID_CELLS] = {0};

  for (int i = 0; i < n_bodies; i++) {
    int c = cell_index_of(grid, &bodies[i]);
    counts[c]++;
  }

  grid->cell_start[0] = 0;
  for (int c = 0; c < grid->grid_cells; c++) {
    grid->cell_start[c + 1] = grid->cell_start[c] + counts[c];
  }

  int cursor[SPATIAL_MAX_GRID_CELLS];
  for (int c = 0; c < grid->grid_cells; c++) {
    cursor[c] = grid->cell_start[c];
  }

  for (int i = 0; i < n_bodies; i++) {
    int c = cell_index_of(grid, &bodies[i]);
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

void calculate_forces_spatial(const Body *bodies, int n_bodies,
                               float *ax, float *ay, const SpatialGrid *grid) {
#ifdef _OPENMP
  #pragma omp parallel for schedule(runtime)
  for (int c = 0; c < grid->grid_cells; c++) {
    int start = grid->cell_start[c];
    int end   = grid->cell_start[c + 1];
    for (int k = start; k < end; k++) {
      int i = grid->cell_bodies[k];
      accumulate_force(bodies, n_bodies, i, &ax[i], &ay[i]);
    }
  }
#else
  for (int c = 0; c < grid->grid_cells; c++) {
    int start = grid->cell_start[c];
    int end   = grid->cell_start[c + 1];
    for (int k = start; k < end; k++) {
      int i = grid->cell_bodies[k];
      accumulate_force(bodies, n_bodies, i, &ax[i], &ay[i]);
    }
  }
#endif
}