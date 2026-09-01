#include <stdlib.h>
#include <math.h>
#include "barnes_hut.h"

#ifdef _OPENMP
#include <omp.h>
#endif

int bh_tree_init(BHTree *tree, int capacity_hint) {
  if (capacity_hint < 16) capacity_hint = 16;
  tree->nodes = malloc((size_t)capacity_hint * sizeof(BHNode));
  tree->capacity = capacity_hint;
  tree->count = 0;
  return tree->nodes != NULL;
}

void bh_tree_free(BHTree *tree) {
  free(tree->nodes);
  tree->nodes = NULL;
  tree->capacity = 0;
  tree->count = 0;
}

static int bh_alloc_node(BHTree *tree) {
  if (tree->count >= tree->capacity) return -1;
  int idx = tree->count++;
  BHNode *n = &tree->nodes[idx];
  n->mass = 0.0f;
  n->com_x = 0.0f;
  n->com_y = 0.0f;
  n->is_leaf = 1;
  n->body = -1;
  n->children[0] = n->children[1] = n->children[2] = n->children[3] = -1;
  return idx;
}

// Determina en cual de los 4 cuadrantes del nodo cae (x, y) y devuelve el
// bounding box de ese cuadrante.
static int bh_quadrant(const BHNode *node, float x, float y,
                        float *out_min_x, float *out_min_y, float *out_size) {
  float half = node->size * 0.5f;
  float mid_x = node->min_x + half;
  float mid_y = node->min_y + half;
  int q;

  if (x < mid_x) {
    if (y < mid_y) { q = 0; *out_min_x = node->min_x; *out_min_y = node->min_y; }
    else           { q = 1; *out_min_x = node->min_x; *out_min_y = mid_y; }
  } else {
    if (y < mid_y) { q = 2; *out_min_x = mid_x; *out_min_y = node->min_y; }
    else           { q = 3; *out_min_x = mid_x; *out_min_y = mid_y; }
  }
  *out_size = half;
  return q;
}

static void bh_insert(BHTree *tree, int node_idx, int b, const Body *bodies, int depth);

static void bh_insert_into_child(BHTree *tree, int parent_idx, int b, const Body *bodies, int depth) {
  BHNode *parent = &tree->nodes[parent_idx];
  float cmin_x, cmin_y, csize;
  int q = bh_quadrant(parent, bodies[b].x, bodies[b].y, &cmin_x, &cmin_y, &csize);

  int child_idx = parent->children[q];
  if (child_idx == -1) {
    child_idx = bh_alloc_node(tree);
    if (child_idx == -1) {
      // Pool de nodos agotado (caso raro con la capacidad recomendada):
      // en vez de desbordar memoria, se funde el cuerpo directamente en
      // las estadisticas del padre. Se pierde precision posicional dentro
      // de ese padre, pero la masa total sigue siendo correcta.
      float total = parent->mass + bodies[b].mass;
      parent->com_x = (parent->com_x * parent->mass + bodies[b].x * bodies[b].mass) / total;
      parent->com_y = (parent->com_y * parent->mass + bodies[b].y * bodies[b].mass) / total;
      parent->mass = total;
      return;
    }
    parent->children[q] = child_idx;
    tree->nodes[child_idx].min_x = cmin_x;
    tree->nodes[child_idx].min_y = cmin_y;
    tree->nodes[child_idx].size = csize;
  }

  bh_insert(tree, child_idx, b, bodies, depth + 1);
}

static void bh_insert(BHTree *tree, int node_idx, int b, const Body *bodies, int depth) {
  BHNode *node = &tree->nodes[node_idx];

  // Salvaguarda: si dos cuerpos caen casi en el mismo punto, evita
  // subdividir indefinidamente y simplemente los funde como un cuerpo
  // compuesto en este nodo.
  if (depth >= BH_MAX_DEPTH) {
    float total = node->mass + bodies[b].mass;
    if (total > 0.0f) {
      node->com_x = (node->com_x * node->mass + bodies[b].x * bodies[b].mass) / total;
      node->com_y = (node->com_y * node->mass + bodies[b].y * bodies[b].mass) / total;
    }
    node->mass = total;
    return;
  }

  if (node->mass == 0.0f) {
    // Nodo recien creado: se convierte en hoja con este cuerpo.
    node->is_leaf = 1;
    node->body = b;
    node->mass = bodies[b].mass;
    node->com_x = bodies[b].x;
    node->com_y = bodies[b].y;
    return;
  }

  if (node->is_leaf) {
    // Hoja ya ocupada: subdividir y reinsertar el cuerpo existente antes
    // de insertar el nuevo. node->mass/com_x/com_y siguen representando
    // (por ahora) solo al cuerpo existente.
    int existing_body = node->body;
    node->is_leaf = 0;
    node->body = -1;

    bh_insert_into_child(tree, node_idx, existing_body, bodies, depth);
    bh_insert_into_child(tree, node_idx, b, bodies, depth);
  } else {
    bh_insert_into_child(tree, node_idx, b, bodies, depth);
  }

  // Actualiza masa/centro de masa acumulados de este nodo (promedio
  // ponderado incremental), ya sea que se haya subdividido recien o que
  // ya fuera un nodo interno.
  node = &tree->nodes[node_idx];
  float total = node->mass + bodies[b].mass;
  node->com_x = (node->com_x * node->mass + bodies[b].x * bodies[b].mass) / total;
  node->com_y = (node->com_y * node->mass + bodies[b].y * bodies[b].mass) / total;
  node->mass = total;
}

void bh_tree_build(BHTree *tree, const Body *bodies, int n_bodies) {
  tree->count = 0;
  if (n_bodies <= 0) return;

  float min_x = bodies[0].x, max_x = bodies[0].x;
  float min_y = bodies[0].y, max_y = bodies[0].y;
  for (int i = 1; i < n_bodies; i++) {
    if (bodies[i].x < min_x) min_x = bodies[i].x;
    if (bodies[i].x > max_x) max_x = bodies[i].x;
    if (bodies[i].y < min_y) min_y = bodies[i].y;
    if (bodies[i].y > max_y) max_y = bodies[i].y;
  }

  const float PADDING = 4.0f;
  float span = fmaxf(max_x - min_x, max_y - min_y) + PADDING;
  if (span <= 0.0f) span = 1.0f;

  int root = bh_alloc_node(tree); // siempre indice 0 (count arranca en 0)
  tree->nodes[root].min_x = min_x - PADDING * 0.5f;
  tree->nodes[root].min_y = min_y - PADDING * 0.5f;
  tree->nodes[root].size = span;

  for (int i = 0; i < n_bodies; i++) {
    bh_insert(tree, root, i, bodies, 0);
  }
}

// Recorre el arbol acumulando la aceleracion sobre el cuerpo self_body
// (ubicado en xi, yi). self_body se usa unicamente para evitar que un
// cuerpo se atraiga a si mismo cuando cae solo en una hoja.
static void bh_accumulate(const BHTree *tree, int node_idx, int self_body,
                           float xi, float yi, float theta,
                           float *out_ax, float *out_ay) {
  const BHNode *node = &tree->nodes[node_idx];
  if (node->mass <= 0.0f) return;

  float dx = node->com_x - xi;
  float dy = node->com_y - yi;
  float dist2 = dx * dx + dy * dy + EPSILON * EPSILON;

  if (node->is_leaf) {
    if (node->body == self_body) return;
    float inv_r3 = 1.0f / (dist2 * sqrtf(dist2));
    float f = CONST_G * node->mass * inv_r3;
    *out_ax += f * dx;
    *out_ay += f * dy;
    return;
  }

  // Criterio de apertura de Barnes-Hut: si el nodo es pequeno comparado
  // con la distancia al cuerpo (size / dist < theta), se aproxima como un
  // unico cuerpo en su centro de masa en vez de recursar en sus hijos.
  // theta mas chico = mas preciso y mas lento; theta=0 equivale a
  // gravedad exacta (nunca se abre la aproximacion).
  float dist = sqrtf(dist2);
  if ((node->size / dist) < theta) {
    float inv_r3 = 1.0f / (dist2 * dist);
    float f = CONST_G * node->mass * inv_r3;
    *out_ax += f * dx;
    *out_ay += f * dy;
    return;
  }

  for (int k = 0; k < 4; k++) {
    if (node->children[k] != -1) {
      bh_accumulate(tree, node->children[k], self_body, xi, yi, theta, out_ax, out_ay);
    }
  }
}

void calculate_forces_barnes_hut(const BHTree *tree, const Body *bodies,
                                  int n_bodies, float *ax, float *ay, float theta) {
  // La profundidad del recorrido por cuerpo varia segun la densidad local
  // (cerca del sol hay muchas mas subdivisiones que en zonas vacias), asi
  // que el trabajo por iteracion es desbalanceado: schedule(dynamic) evita
  // que un hilo con cuerpos "faciles" espere ocioso a uno con cuerpos en
  // zonas densas.
  #pragma omp parallel for schedule(dynamic, 32)
  for (int i = 0; i < n_bodies; i++) {
    float sum_ax = 0.0f;
    float sum_ay = 0.0f;
    bh_accumulate(tree, 0, i, bodies[i].x, bodies[i].y, theta, &sum_ax, &sum_ay);
    ax[i] = sum_ax;
    ay[i] = sum_ay;
  }
}