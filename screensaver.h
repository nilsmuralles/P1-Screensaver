#define WIDTH 640
#define HEIGHT 480
#define MIN_FPS 30

#define CONST_G 6.674e-11

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

void init_bodies(Body *bodies, int num_bodies);
