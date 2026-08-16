#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "screensaver.h"

void init_bodies(Body *bodies, int n_bodies) {
  // Sol
  bodies[0].x = WIDTH / 2.0f;
  bodies[0].y = HEIGHT / 2.0f;
  bodies[0].vx = 0.0f;
  bodies[0].vy = 0.0f;
  bodies[0].mass = 5000.0f;
  bodies[0].radius = 8.0f;
  bodies[0].r = 255; 
  bodies[0].g = 220; 
  bodies[0].b = 100;

  for (int i = 1; i < n_bodies; i++) { // Resto de cuerpos celestes
    // Posición
    float angle = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
    float dist  = 30.0f + ((float)rand() / RAND_MAX) * (WIDTH / 2.0f - 30.0f);
    bodies[i].x = WIDTH / 2.0f + dist * cosf(angle);
    bodies[i].y = HEIGHT / 2.0f + dist * sinf(angle);
    
    // Velocidad
    float speed = sqrtf(CONST_G * bodies[0].mass / dist);
    bodies[i].vx = -sinf(angle) * speed;
    bodies[i].vy =  cosf(angle) * speed;

    // Tamaño
    bodies[i].mass = 1.0f + ((float)rand() / RAND_MAX) * 4.0f;
    bodies[i].radius = 1.5f + bodies[i].mass * 0.3f;

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

void update_bodies(Body *bodies, int n_bodies, float *ax, float *ay, float dt) {
  for (int i = 0; i < n_bodies; i++) {
    bodies[i].vx += ax[i] * dt;
    bodies[i].vy += ay[i] * dt;

    bodies[i].x += bodies[i].vx * dt;
    bodies[i].y += bodies[i].vy * dt;
  }
}
