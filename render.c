#include <math.h>
#include "raylib.h"
#include "render.h"

// Valor por defecto de estela
#define DEFAULT_TRAIL_ALPHA 40

int render_init(RenderContext *ctx, const char *title, int width, int height) {

  SetTraceLogLevel(LOG_WARNING);

  InitWindow(width, height, title);

  if (!IsWindowReady()) {
    return 0;
  }

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

void render_present(RenderContext *ctx) {
  (void)ctx;

  DrawRectangle(5, 5, 95, 25, (Color){ 0, 0, 0, 160 }); // fondo para legibilidad
  DrawFPS(10, 10); // helper de raylib

  EndDrawing();
}

void render_update_fps(RenderContext *ctx) {
  // Solo actualiza el estado interno para quien consulte render_get_fps()
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