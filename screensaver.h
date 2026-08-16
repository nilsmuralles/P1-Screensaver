#define WIDTH 640
#define HEIGHT 480
#define MIN_FPS 30

#define CONST_G 6.674e-2
#define EPSILON 2.0f

enum ExecMode {
  SECUENCIAL
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
