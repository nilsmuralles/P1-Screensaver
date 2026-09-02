#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "screensaver.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void init_bodies(Body *bodies, int n_bodies) {
  // Sol
  bodies[0].x = WIDTH / 2.0f;
  bodies[0].y = HEIGHT / 2.0f;
  bodies[0].vx = 0.0f;
  bodies[0].vy = 0.0f;
  bodies[0].mass = SUN_BASE_MASS;
  bodies[0].radius = SUN_BASE_RADIUS;
  bodies[0].r = 255;
  bodies[0].g = 220; 
  bodies[0].b = 100;

  float min_orbit_dist = SUN_BASE_RADIUS * (MERGE_DISTANCE_FACTOR + 1.0f) + 20.0f;

  for (int i = 1; i < n_bodies; i++) { // Resto de cuerpos celestes
    // Posición
    float angle = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
    float dist  = min_orbit_dist + ((float)rand() / RAND_MAX) * (WIDTH / 2.0f - min_orbit_dist);
    bodies[i].x = WIDTH / 2.0f + dist * cosf(angle);
    bodies[i].y = HEIGHT / 2.0f + dist * sinf(angle);
    
    float circular_speed = sqrtf(CONST_G * bodies[0].mass / dist);
    float speed_factor = 0.5f + ((float)rand() / RAND_MAX) * 0.4f;
    float speed = circular_speed * speed_factor;
    bodies[i].vx = -sinf(angle) * speed;
    bodies[i].vy =  cosf(angle) * speed;

    // Tamaño
    bodies[i].mass = 1.0f + ((float)rand() / RAND_MAX) * 4.0f;
    bodies[i].radius = 2.0f + bodies[i].mass * 1.0f;

    // Color
    bodies[i].r = rand() % 256;
    bodies[i].g = rand() % 256;
    bodies[i].b = rand() % 256;
  }
}

void calculate_forces(Body *bodies, int n_bodies, float *ax, float *ay) {
  for (int i = 0; i < n_bodies; i++) {
    float sum_ax = 0.0f;
    float sum_ay = 0.0f;

    for (int j = 0; j < n_bodies; j++) {
      if (i == j) continue;
      
      float dx = bodies[j].x - bodies[i].x;
      float dy = bodies[j].y - bodies[i].y;
      float r2 = dx*dx + dy*dy + EPSILON*EPSILON;
      float r  = sqrtf(r2);

      float f = CONST_G * bodies[j].mass / (r2 * r);

      sum_ax += f * dx;
      sum_ay += f * dy;
      
    } 

    ax[i] = sum_ax;
    ay[i] = sum_ay;
  }
}

#ifdef _OPENMP
static omp_sched_t g_forces_schedule_kind = omp_sched_static;
static int g_forces_schedule_chunk = 0;
#endif

void set_forces_schedule(int schedule_kind) {
#ifdef _OPENMP
  switch (schedule_kind) {
    case 1: g_forces_schedule_kind = omp_sched_dynamic; break;
    case 2: g_forces_schedule_kind = omp_sched_guided; break;
    default: g_forces_schedule_kind = omp_sched_static; break;
  }
  omp_set_schedule(g_forces_schedule_kind, g_forces_schedule_chunk);
#else
  (void)schedule_kind;
#endif
}

const char *forces_schedule_name(void) {
#ifdef _OPENMP
  switch (g_forces_schedule_kind) {
    case omp_sched_dynamic: return "dynamic";
    case omp_sched_guided:  return "guided";
    default:                return "static";
  }
#else
  return "secuencial (sin OpenMP)";
#endif
}

// Estrategia 1 (por datos): mismo algoritmo O(N^2) que calculate_forces,
// pero repartiendo el loop externo de "i" entre hilos con OpenMP. Cada
// iteracion de i solo escribe ax[i]/ay[i], por eso no hace falta
// critical/atomic ni reduction.
void calculate_forces_parallel(Body *bodies, int n_bodies, float *ax, float *ay) {
  #pragma omp parallel for schedule(runtime)
  for (int i = 0; i < n_bodies; i++) {
    // bodies[i].x/y se cargan una sola vez fuera del bucle interno en vez
    // de releerlos de memoria en cada una de las N-1 iteraciones de j.
    const float xi = bodies[i].x;
    const float yi = bodies[i].y;
    float sum_ax = 0.0f;
    float sum_ay = 0.0f;

    for (int j = 0; j < n_bodies; j++) {
      if (i == j) continue;

      float dx = bodies[j].x - xi;
      float dy = bodies[j].y - yi;
      float r2 = dx*dx + dy*dy + EPSILON*EPSILON;
      float r  = sqrtf(r2);

      float f = CONST_G * bodies[j].mass / (r2 * r);

      sum_ax += f * dx;
      sum_ay += f * dy;
    }

    ax[i] = sum_ax;
    ay[i] = sum_ay;
  }
}

void simulate_step_datos(Body *bodies, int n_bodies, float *ax, float *ay, float dt) {
  #pragma omp parallel
  {
    #pragma omp for schedule(runtime)
    for (int i = 0; i < n_bodies; i++) {
      const float xi = bodies[i].x;
      const float yi = bodies[i].y;
      float sum_ax = 0.0f;
      float sum_ay = 0.0f;

      for (int j = 0; j < n_bodies; j++) {
        if (i == j) continue;

        float dx = bodies[j].x - xi;
        float dy = bodies[j].y - yi;
        float r2 = dx*dx + dy*dy + EPSILON*EPSILON;
        float f  = CONST_G * bodies[j].mass / (r2 * sqrtf(r2));

        sum_ax += f * dx;
        sum_ay += f * dy;
      }

      ax[i] = sum_ax;
      ay[i] = sum_ay;
    }

    // Barrera implicita del "omp for" de arriba.
    #pragma omp for schedule(runtime)
    for (int i = 1; i < n_bodies; i++) {
      bodies[i].vx += ax[i] * dt;
      bodies[i].vy += ay[i] * dt;
      bodies[i].x += bodies[i].vx * dt;
      bodies[i].y += bodies[i].vy * dt;
    }
  }
}

// Tamaño de bloque por task: 
//    Muy chico => overhead de crear tasks domina.
//    Muy grande => pocos tasks, mal balanceo. 
// 256 es un punto de partida, razonable para N en el rango de miles/decenas de miles.
#define TASK_CHUNK_SIZE 256

// Genera un task por bloque de cuerpos. Debe llamarse desde dentro de una
// region "#pragma omp parallel" + "single" ya abierta por el caller, para poder 
// reutilizarse tanto sola como encadenada con otros tasks sin caer en paralelismo anidado.
static void dispatch_force_tasks(Body *bodies, int n_bodies, float *ax, float *ay) {
  for (int start = 0; start < n_bodies; start += TASK_CHUNK_SIZE) {
    int end = start + TASK_CHUNK_SIZE;
    if (end > n_bodies) end = n_bodies;

    #pragma omp task firstprivate(start, end) shared(bodies, ax, ay, n_bodies)
    {
      for (int i = start; i < end; i++) {
        const float xi = bodies[i].x;
        const float yi = bodies[i].y;
        float sum_ax = 0.0f;
        float sum_ay = 0.0f;

        for (int j = 0; j < n_bodies; j++) {
          if (i == j) continue;
          float dx = bodies[j].x - xi;
          float dy = bodies[j].y - yi;
          float r2 = dx*dx + dy*dy + EPSILON*EPSILON;
          float f  = CONST_G * bodies[j].mass / (r2 * sqrtf(r2));
          sum_ax += f * dx;
          sum_ay += f * dy;
        }

        ax[i] = sum_ax;
        ay[i] = sum_ay;
      }
    } // fin task
  }
}

// Estrategia 3 (por tareas): en vez de un omp for con schedule fijo, el
// hilo "single" genera explicitamente un task por bloque de cuerpos y el
// runtime de OpenMP los reparte dinamicamente entre los hilos libres.
void calculate_forces_tasks(Body *bodies, int n_bodies, float *ax, float *ay) {
  #pragma omp parallel
  {
    #pragma omp single
    {
      dispatch_force_tasks(bodies, n_bodies, ax, ay);
      #pragma omp taskwait
    }
  }
}

// Pipeline por tareas: fuerzas y actualizacion dentro de una sola region
// parallel/single (evita abrir una segunda region por frame, igual que
// simulate_step_datos). update_bodies depende del resultado de las tasks
// de fuerza, por eso el "taskwait" intermedio antes de actualizarlas.
void simulate_step_tasks(Body *bodies, int n_bodies, float *ax, float *ay, float dt) {
  #pragma omp parallel
  {
    #pragma omp single
    {
      dispatch_force_tasks(bodies, n_bodies, ax, ay);
      #pragma omp taskwait

      update_bodies(bodies, n_bodies, ax, ay, dt);
    }
  }
}

void update_bodies(Body *bodies, int n_bodies, float *ax, float *ay, float dt) {
  for (int i = 1; i < n_bodies; i++) {
    bodies[i].vx += ax[i] * dt;
    bodies[i].vy += ay[i] * dt;

    bodies[i].x += bodies[i].vx * dt;
    bodies[i].y += bodies[i].vy * dt;
  }
}

int check_and_merge_collisions(Body *bodies, int n_active) {
  int i = 1; 

  while (i < n_active) {
    float dx = bodies[i].x - bodies[0].x;
    float dy = bodies[i].y - bodies[0].y;
    float dist2 = dx * dx + dy * dy;

    float merge_dist = (bodies[0].radius + bodies[i].radius) * MERGE_DISTANCE_FACTOR;

    if (dist2 <= merge_dist * merge_dist) {
      bodies[0].mass += bodies[i].mass;

      bodies[i] = bodies[n_active - 1]; 
      n_active--;
    } else {
      i++;
    }
  }

  bodies[0].radius = SUN_BASE_RADIUS * cbrtf(bodies[0].mass / SUN_BASE_MASS);

  return n_active;
}

float total_planet_mass(const Body *bodies, int n_active) {
  float total = 0.0f;
  for (int i = 1; i < n_active; i++) {
    total += bodies[i].mass;
  }
  return total;
}

int should_explode(const Body *bodies, int n_active, float mass_threshold) {
  if (bodies[0].mass >= mass_threshold) return 1;
  if (n_active <= 1) return 1;
  return 0;
}