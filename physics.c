#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "screensaver.h"

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
