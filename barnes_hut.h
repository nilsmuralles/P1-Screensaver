#ifndef BARNES_HUT_H
#define BARNES_HUT_H

#include "screensaver.h"

// Aproximacion jerarquica O(N log N): agrupa cuerpos lejanos en un solo
// nodo (masa total + centro de masa) y solo abre la recursion cuando el
// nodo es "grande" en relacion a la distancia (criterio de apertura theta).
// A diferencia de la estrategia espacial actual (spatial.c), esto SI
// reduce el numero de interacciones evaluadas; no calcula gravedad exacta.

#define BH_THETA_DEFAULT 0.5f
#define BH_MAX_DEPTH 32

typedef struct {
  // Bounding box cuadrado de este nodo: esquina inferior (min_x, min_y),
  // lado "size".
  float min_x, min_y, size;

  // Masa total y centro de masa acumulados durante la construccion.
  float mass;
  float com_x, com_y;

  int children[4]; // indices en el pool del arbol, -1 si no existe
  int body;         // indice del cuerpo si el nodo es una hoja con un
                     // unico cuerpo; -1 si es interno o esta vacio
  int is_leaf;
} BHNode;

typedef struct {
  BHNode *nodes;
  int capacity;
  int count;
} BHTree;

// Reserva el pool de nodos. capacity_hint es una cota superior estimada
// (se recomienda 8 * n_bodies + 16); si se agota durante la insercion, el
// arbol degrada de forma segura fusionando cuerpos restantes en el nodo
// actual en vez de desbordar memoria.
int bh_tree_init(BHTree *tree, int capacity_hint);
void bh_tree_free(BHTree *tree);

// Construye el arbol desde cero a partir de las posiciones actuales.
// Debe llamarse una vez por paso de simulacion, antes de calcular fuerzas.
void bh_tree_build(BHTree *tree, const Body *bodies, int n_bodies);

// Calcula la aceleracion de cada cuerpo recorriendo el arbol. Paraleliza
// el bucle externo por cuerpo destino (cada recorrido del arbol es de solo
// lectura e independiente entre cuerpos).
void calculate_forces_barnes_hut(const BHTree *tree, const Body *bodies,
                                  int n_bodies, float *ax, float *ay, float theta);

#endif