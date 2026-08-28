#ifndef SPATIAL_H
#define SPATIAL_H

#include "screensaver.h"

#define SPATIAL_GRID_COLS 8
#define SPATIAL_GRID_ROWS 8
#define SPATIAL_GRID_CELLS (SPATIAL_GRID_COLS * SPATIAL_GRID_ROWS)

typedef enum {
  SPATIAL_SCHEDULE_STATIC,
  SPATIAL_SCHEDULE_DYNAMIC,
  SPATIAL_SCHEDULE_GUIDED
} SpatialScheduleKind;

typedef struct {
  int capacity;          // tamano maximo reservado
  int cell_start[SPATIAL_GRID_CELLS + 1];
  int *cell_bodies;       // indices de cuerpos agrupados por celda
  float cell_width;
  float cell_height;
} SpatialGrid;

// Reserva las estructuras internas del grid para hasta max_bodies cuerpos.
// Devuelve 1 en exito, 0 si malloc falla.
int spatial_grid_init(SpatialGrid *grid, int max_bodies);

// Libera la memoria reservada por spatial_grid_init.
void spatial_grid_free(SpatialGrid *grid);

// Reconstruye el bucketing del grid a partir de las posiciones actuales de los cuerpos. 
void spatial_grid_build(SpatialGrid *grid, const Body *bodies, int n_bodies);

// Configura la politica de reparto de OpenMP que usara calculate_forces_spatial
// Sin OpenMP habilitado esta funcion no hace nada.
void spatial_set_schedule(SpatialScheduleKind kind, int chunk_size);

// Nombre legible de la politica de schedule activa (para logs/benchmark).
const char *spatial_schedule_name(void);

// Calcula la aceleracion de cada cuerpo igual que calculate_forces() de
// physics.c pero repartiendo el trabajo por celda del grid entre hilos en vez de por
// indice plano del arreglo.
void calculate_forces_spatial(const Body *bodies, int n_bodies, float *ax, float *ay, const SpatialGrid *grid);

#endif