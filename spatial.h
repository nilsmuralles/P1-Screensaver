#ifndef SPATIAL_H
#define SPATIAL_H

#include "screensaver.h"


// Como los cuerpos se concentran cerca del sol, las celdas quedan
// naturalmente desbalanceadas en cantidad de cuerpos ese
#define SPATIAL_MAX_GRID_COLS 32
#define SPATIAL_MAX_GRID_ROWS 32
#define SPATIAL_MAX_GRID_CELLS (SPATIAL_MAX_GRID_COLS * SPATIAL_MAX_GRID_ROWS)
#define SPATIAL_BODIES_PER_CELL_TARGET 4

typedef enum {
  SPATIAL_SCHEDULE_STATIC,
  SPATIAL_SCHEDULE_DYNAMIC,
  SPATIAL_SCHEDULE_GUIDED
} SpatialScheduleKind;

typedef struct {
  int capacity;          // tamano maximo reservado (>= MAX_BODIES esperado)
  int grid_cols;
  int grid_rows;
  int grid_cells;        // grid_cols * grid_rows, <= SPATIAL_MAX_GRID_CELLS
  int cell_start[SPATIAL_MAX_GRID_CELLS + 1]; // offsets al estilo CSR/counting-sort
  int *cell_bodies;       // indices de cuerpos agrupados por celda, tamano=capacity

  // Ajustar al bounding box mantiene las celdas utiles sin importar cuanto se hayan agrupado los cuerpos.
  float origin_x;
  float origin_y;
  float cell_width;
  float cell_height;
} SpatialGrid;

// Reserva las estructuras internas del grid para hasta max_bodies cuerpos.
// Devuelve 1 en exito, 0 si malloc falla.
int spatial_grid_init(SpatialGrid *grid, int max_bodies);

// Libera la memoria reservada por spatial_grid_init.
void spatial_grid_free(SpatialGrid *grid);

// Reconstruye el bucketing del grid a partir de las posiciones actuales de
// los cuerpos
void spatial_grid_build(SpatialGrid *grid, const Body *bodies, int n_bodies);

// Configura la politica de reparto de OpenMP que usara calculate_forces_spatial
void spatial_set_schedule(SpatialScheduleKind kind, int chunk_size);

// Nombre legible de la politica de schedule activa (para logs/benchmark).
const char *spatial_schedule_name(void);

// Calcula la aceleracion de cada cuerpo igual que calculate_forces() de
// physics.c (misma formula, mismo CONST_G/EPSILON, sin radio de corte),
// pero repartiendo el trabajo por celda del grid entre hilos en vez de por
// indice plano del arreglo. Requiere spatial_grid_build() ya ejecutado
// sobre el mismo n_bodies.
void calculate_forces_spatial(const Body *bodies, int n_bodies, float *ax, float *ay, const SpatialGrid *grid);

#endif