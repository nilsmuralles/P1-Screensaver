#ifndef SCREENSAVER_H
#define SCREENSAVER_H

#define WIDTH 640
#define HEIGHT 480
#define MIN_FPS 30
#define TARGET_FPS 60 // referencia historica; ya no se usa para limitar FPS (ver decision 6.3 del plan)

// Escala de tiempo simulado por segundo real. Con dt = GetFrameTime() * SIM_TIME_SCALE,
// a exactamente 60 FPS reales dt queda numericamente igual al dt=1.0f fijo que se usaba
// antes, asi que las orbitas se ven igual que en la calibracion original, pero ahora
// avanzan a ritmo de tiempo real (frame-rate independiente) en vez de "1 tick por frame".
// Por calibrar visualmente junto al equipo si se ajustan CONST_G/SUN_BASE_MASS.
#define SIM_TIME_SCALE 60.0f

#define CONST_G 6.674e-2
#define EPSILON 2.0f
#define MAX_BODIES 100000

// Ciclo de vida de la estrella: acrecion (fusion planeta-sol) + explosion
#define SUN_BASE_MASS 5000.0f
#define SUN_BASE_RADIUS 12.0f
#define MERGE_DISTANCE_FACTOR 1.8f       // holgura sobre la suma de radios al fusionar, por calibrar
#define EXPLOSION_ABSORPTION_FRACTION 0.3f // fraccion de la masa total de planetas que el sol debe absorber para explotar
#define EXPLOSION_DURATION_FRAMES 60      // duracion del efecto visual antes de reiniciar, por calibrar

enum ExecMode {
  SECUENCIAL,
  ESPACIAL

};

enum SimState {
  STATE_RUNNING,
  STATE_EXPLODING
};

typedef struct {
  float radius;
  float x, y;
  float vx, vy;
  float mass;
  unsigned char r, g, b;
} Body;

void init_bodies(Body *bodies, int n_boides);
void calculate_forces(Body *bodies, int n_bodies, float *ax, float *ay);
void update_bodies(Body *bodies, int n_bodies, float *ax, float *ay, float dt);
int parse_args(int argc, char *argv[], int *n_bodies, enum ExecMode *mode, int *schedule_kind);
int check_and_merge_collisions(Body *bodies, int n_active);
int should_explode(const Body *bodies, int n_active, float mass_threshold);
float total_planet_mass(const Body *bodies, int n_active);

#endif