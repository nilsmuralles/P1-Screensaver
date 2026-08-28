#include <math.h>
#include "raylib.h"
#include "render.h"

#define DEFAULT_TRAIL_ALPHA 40

int render_init(RenderContext *ctx, const char *title, int width, int height) {

  SetTraceLogLevel(LOG_WARNING);

  InitWindow(width, height, title);

  if (!IsWindowReady()) {
    return 0;
  }

  SetTargetFPS(TARGET_FPS);

  ctx->width  = width;
  ctx->height = height;
  ctx->running = 1;

  ctx->bg_r = 10;
  ctx->bg_g = 10;
  ctx->bg_b = 20;
  ctx->trail_alpha = DEFAULT_TRAIL_ALPHA;

  ctx->fps_current = 0.0;

  return 1;
}

void render_shutdown(RenderContext *ctx) {
  (void)ctx;
  CloseWindow();
}

int render_poll_events(RenderContext *ctx) {

  if (WindowShouldClose()) {
    ctx->running = 0;
  }
  return ctx->running;
}

void render_clear(RenderContext *ctx, unsigned char r, unsigned char g, unsigned char b) {
  ctx->bg_r = r;
  ctx->bg_g = g;
  ctx->bg_b = b;

  BeginDrawing();

  Color bg = (Color){ r, g, b, ctx->trail_alpha };
  DrawRectangle(0, 0, ctx->width, ctx->height, bg);
}

void render_bodies(RenderContext *ctx, const Body *bodies, int n_bodies) {
  (void)ctx;
  for (int i = 0; i < n_bodies; i++) {
    const Body *body = &bodies[i];
    Color color = (Color){ body->r, body->g, body->b, 255 };
    DrawCircle((int)roundf(body->x), (int)roundf(body->y), body->radius, color);
  }
}

void render_explosion_effect(RenderContext *ctx, const Body *bodies, int n_active, int frames_remaining) {
  (void)n_active;

  float progress = 1.0f - ((float)frames_remaining / (float)EXPLOSION_DURATION_FRAMES);
  if (progress < 0.0f) progress = 0.0f;
  if (progress > 1.0f) progress = 1.0f;

  Vector2 center = { bodies[0].x, bodies[0].y };
  unsigned char alpha = (unsigned char)(255.0f * (1.0f - progress));

  float core_radius = bodies[0].radius * (1.0f - progress) + 4.0f;
  Color core = (Color){ 255, 240, 200, alpha };
  DrawCircleV(center, core_radius, core);

  float max_radius = ctx->width * 0.6f;
  float ring_radius = bodies[0].radius + progress * max_radius;
  Color ring = (Color){ 255, 140, 30, alpha };
  DrawRing(center, ring_radius * 0.85f, ring_radius, 0, 360, 64, ring);
}

void render_present(RenderContext *ctx) {
  (void)ctx;

  DrawRectangle(5, 5, 95, 25, (Color){ 0, 0, 0, 160 }); // fondo para legibilidad
  DrawFPS(10, 10); 

  EndDrawing();
}

void render_update_fps(RenderContext *ctx) {
  ctx->fps_current = (double)GetFPS();
}

double render_get_fps(const RenderContext *ctx) {
  return ctx->fps_current;
}

void render_set_trail(RenderContext *ctx, unsigned char alpha) {
  ctx->trail_alpha = alpha;
}

unsigned char rand_color_channel(void) {
  return (unsigned char)GetRandomValue(0, 255);
}
