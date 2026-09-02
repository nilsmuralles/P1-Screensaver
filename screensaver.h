#ifndef SCREENSAVER_H
#define SCREENSAVER_H

#define WIDTH 1280
#define HEIGHT 960
#define MIN_FPS 30
#define TARGET_FPS 60 // referencia historica; ya no se usa para limitar FPS (ver decision 6.3 del plan)

// Escala de tiempo simulado por segundo real. Con dt = GetFrameTime() * SIM_TIME_SCALE,
// a exactamente 60 FPS reales dt queda numericamente igual al dt=1.0f fijo que se usaba
// antes, asi que las orbitas se ven igual que en la calibracion original, pero ahora
// avanzan a ritmo de tiempo real (frame-rate independiente) en vez de "1 tick por frame".
// Por calibrar visualmente junto al equipo si se ajustan CONST_G/SUN_BASE_MASS.
#define SIM_TIME_SCALE 60.0f

#define CONST_G 6.674e-2f
#define EPSILON 2.0f
#define MAX_BODIES 100000

#define SUN_BASE_MASS 5000.0f
#define SUN_BASE_RADIUS 22.0f
#define MERGE_DISTANCE_FACTOR 1.8f
#define EXPLOSION_ABSORPTION_FRACTION 0.3f
#define EXPLOSION_DURATION_SECONDS 2.5f

enum ExecMode {
  SECUENCIAL,
  PARALELO_DATOS,
  ESPACIAL,
  TAREAS
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
void calculate_forces_parallel(Body *bodies, int n_bodies, float *ax, float *ay);
void set_forces_schedule(int schedule_kind);
const char *forces_schedule_name(void);
void update_bodies(Body *bodies, int n_bodies, float *ax, float *ay, float dt);
int parse_args(int argc, char *argv[], int *n_bodies, enum ExecMode *mode, int *schedule_kind);
int check_and_merge_collisions(Body *bodies, int n_active);
int should_explode(const Body *bodies, int n_active, float mass_threshold);
float total_planet_mass(const Body *bodies, int n_active);

// --- Estrategia por datos: fuerzas + actualizacion en una sola region
// paralela (evita abrir dos regiones OpenMP por frame). Usa la misma
// politica de schedule configurada via set_forces_schedule().
void simulate_step_datos(Body *bodies, int n_bodies, float *ax, float *ay, float dt);

// --- Tercera ley de Newton: cada pareja (i, j) se calcula una sola vez.
// Usa acumuladores privados por hilo para evitar atomics/critical; el
// costo extra es memoria O(P*N) y una reduccion final O(P*N).
void calculate_forces_parallel_newton3(Body *bodies, int n_bodies, float *ax, float *ay);

// --- Estrategia por tareas: el hilo "single" genera un task por cada
// bloque de cuerpos; el runtime de OpenMP reparte esos tasks entre los
// hilos disponibles (a diferencia de "datos", que usa omp for con
// particionamiento decidido por la clausula schedule, no por el
// programador creando tasks explicitos).
void calculate_forces_tasks(Body *bodies, int n_bodies, float *ax, float *ay);
void simulate_step_tasks(Body *bodies, int n_bodies, float *ax, float *ay, float dt);

// --- Estructura de arreglos (SoA) para el kernel gravitacional: mejora
// localidad de cache frente a cargar todo el struct Body (que incluye
// velocidad, radio y color, no usados por el kernel).
typedef struct {
  float *x;
  float *y;
  float *mass;
  int capacity;
} PhysicsSoA;

int physics_soa_init(PhysicsSoA *soa, int max_bodies);
void physics_soa_free(PhysicsSoA *soa);
void physics_soa_sync(PhysicsSoA *soa, const Body *bodies, int n_bodies);
void calculate_forces_soa_parallel(const PhysicsSoA *soa, int n_bodies, float *ax, float *ay);

#endif