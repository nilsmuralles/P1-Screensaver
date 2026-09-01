#ifndef SPATIAL_H
#define SPATIAL_H

#include "screensaver.h"


// El numero de celdas ahora se calcula en spatial_grid_init() a partir de
// max_bodies (celdas ~= N / SPATIAL_BODIES_PER_CELL_TARGET), en vez de un
// grid fijo de 32x32 = 1024 celdas. Con el grid fijo, N=100000 daba un
// promedio de ~97.7 cuerpos por celda en vez de los 4 objetivo, porque
// 1024 celdas era el techo sin importar cuanto creciera N.
#define SPATIAL_BODIES_PER_CELL_TARGET 4
#define SPATIAL_MIN_GRID_SIDE 1
// Cota de seguridad (no una limitacion de diseno): evita reservar un
// numero absurdo de celdas si max_bodies es enorme. Con este techo el
// grid puede tener hasta 2048*2048 ~= 4.2M celdas.
#define SPATIAL_MAX_GRID_SIDE 2048

typedef enum {
  SPATIAL_SCHEDULE_STATIC,
  SPATIAL_SCHEDULE_DYNAMIC,
  SPATIAL_SCHEDULE_GUIDED
} SpatialScheduleKind;

typedef struct {
  int capacity;          // tamano maximo reservado (>= MAX_BODIES esperado)
  int grid_cols;
  int grid_rows;
  int grid_cells;        // grid_cols * grid_rows, dimensionado segun capacity
  int *cell_start;        // offsets al estilo CSR/counting-sort, tamano grid_cells+1
  int *cell_counts;       // scratch de conteo reutilizado en cada build, tamano grid_cells
  int *cell_bodies;       // indices de cuerpos agrupados por celda, tamano=capacity

  // Ajustar al bounding box mantiene las celdas utiles sin importar cuanto se hayan agrupado los cuerpos.
  float origin_x;
  float origin_y;
  float cell_width;
  float cell_height;
} SpatialGrid;

// Reserva las estructuras internas del grid para hasta max_bodies cuerpos,
// incluyendo cell_start y cell_counts, cuyo tamano ahora depende de
// max_bodies en vez de estar fijo a 32x32.
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