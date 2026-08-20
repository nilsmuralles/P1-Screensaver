#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include "screensaver.h"


typedef struct {
  SDL_Window   *window;
  SDL_Renderer *renderer;
  int           width;
  int           height;
  int           running; // 0 cuando el usuario pidio salir

  // Estado interno para el calculo de FPS
  Uint32 fps_window_start_ms;
  int    fps_frame_count;
  double fps_current;
} RenderContext;

// Inicializa SDL, crea ventana y renderer
int render_init(RenderContext *ctx, const char *title, int width, int height);

// Libera renderer y ventana, hace SDL_Quit()
void render_shutdown(RenderContext *ctx);

// Procesa la cola de eventos (quit, ESC) y devuelve ctx->running
// pensado para usarse como condicion del while del loop principal
int render_poll_events(RenderContext *ctx);

// Limpia el frame con un color de fondo solido.
void render_clear(RenderContext *ctx, unsigned char r, unsigned char g, unsigned char b);

// Dibuja el arreglo de cuerpos como circulos rellenos usando su color y radio.
void render_bodies(RenderContext *ctx, const Body *bodies, int n_bodies);

// Presenta el frame (swap de buffer).
void render_present(RenderContext *ctx);

// Actualiza el contador de FPS y lo refleja en el titulo de la ventana
void render_update_fps(RenderContext *ctx);

// Ultimo valor de FPS calculado
double render_get_fps(const RenderContext *ctx);

// Un canal de color pseudoaleatorio [0-255].
unsigned char rand_color_channel(void);

#endif
