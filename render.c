#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "render.h"

// Dibuja un circulo relleno usando lineas horizontales
static void fill_circle(SDL_Renderer *renderer, int cx, int cy, int radius) {
  if (radius < 1) radius = 1;

  for (int dy = -radius; dy <= radius; dy++) {
    int dx = (int)sqrtf((float)(radius * radius - dy * dy));
    SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
  }
}

int render_init(RenderContext *ctx, const char *title, int width, int height) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "Error SDL_Init: %s\n", SDL_GetError());
    return 0;
  }

  ctx->window = SDL_CreateWindow(
      title,
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width, height,
      SDL_WINDOW_SHOWN);

  if (!ctx->window) {
    fprintf(stderr, "Error SDL_CreateWindow: %s\n", SDL_GetError());
    SDL_Quit();
    return 0;
  }

  ctx->renderer = SDL_CreateRenderer(
      ctx->window, -1,
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!ctx->renderer) {
    // Fallback por si el driver acelerado/vsync no esta disponible
    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_SOFTWARE);
  }

  if (!ctx->renderer) {
    fprintf(stderr, "Error SDL_CreateRenderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(ctx->window);
    SDL_Quit();
    return 0;
  }

  // Blend mode alpha habilitado 
  SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);

  ctx->width  = width;
  ctx->height = height;
  ctx->running = 1;

  ctx->fps_window_start_ms = SDL_GetTicks();
  ctx->fps_frame_count = 0;
  ctx->fps_current = 0.0;

  return 1;
}

void render_shutdown(RenderContext *ctx) {
  if (ctx->renderer) {
    SDL_DestroyRenderer(ctx->renderer);
    ctx->renderer = NULL;
  }
  if (ctx->window) {
    SDL_DestroyWindow(ctx->window);
    ctx->window = NULL;
  }
  SDL_Quit();
}

int render_poll_events(RenderContext *ctx) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      ctx->running = 0;
    } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
      ctx->running = 0;
    }
  }
  return ctx->running;
}

void render_clear(RenderContext *ctx, unsigned char r, unsigned char g, unsigned char b) {
  SDL_SetRenderDrawColor(ctx->renderer, r, g, b, 255);
  SDL_RenderClear(ctx->renderer);
}

void render_bodies(RenderContext *ctx, const Body *bodies, int n_bodies) {
  for (int i = 0; i < n_bodies; i++) {
    const Body *b = &bodies[i];
    SDL_SetRenderDrawColor(ctx->renderer, b->r, b->g, b->b, 255);
    fill_circle(ctx->renderer, (int)roundf(b->x), (int)roundf(b->y), (int)roundf(b->radius));
  }
}

void render_present(RenderContext *ctx) {
  SDL_RenderPresent(ctx->renderer);
}

void render_update_fps(RenderContext *ctx) {
  ctx->fps_frame_count++;

  Uint32 now = SDL_GetTicks();
  Uint32 elapsed_ms = now - ctx->fps_window_start_ms;

  if (elapsed_ms >= 500) {
    ctx->fps_current = (double)ctx->fps_frame_count * 1000.0 / (double)elapsed_ms;
    ctx->fps_frame_count = 0;
    ctx->fps_window_start_ms = now;

    char title[128];
    snprintf(title, sizeof(title), "N-Body Screensaver - %.1f FPS", ctx->fps_current);
    SDL_SetWindowTitle(ctx->window, title);
  }
}

double render_get_fps(const RenderContext *ctx) {
  return ctx->fps_current;
}

unsigned char rand_color_channel(void) {
  return (unsigned char)(rand() % 256);
}