#ifndef RENDER_H
#define RENDER_H

#include "screensaver.h"

// Detalles de shaders ocultos detras de un puntero opaco:
// render.h no debe filtrar tipos de raylib
typedef struct RenderShaderState RenderShaderState;

typedef struct {
  int width;
  int height;
  int running; // 0 cuando el usuario pidio salir

  // Ultimo color de fondo pedido via render_clear() y se reutiliza cada
  // frame para dibujar el rectangulo translucido que genera una estela.
  unsigned char bg_r, bg_g, bg_b;

  // Alpha del rectangulo de fondo por frame mientras mas alta menos se ve
  unsigned char trail_alpha;

  double fps_current;

  RenderShaderState *shaders; // NULL si los shaders no se pudieron inicializar
} RenderContext;

// Inicializa la ventana raylib. Devuelve 1 en exito, 0 en error
int render_init(RenderContext *ctx, const char *title, int width, int height);

// Cierra la ventana raylib
void render_shutdown(RenderContext *ctx);

// Procesa eventos de ventana o teclado. Devuelve ctx->running,
int render_poll_events(RenderContext *ctx);

// Limpia el frame, internamente dibuja un rectangulo del color pedido
// con alpha = ctx->trail_alpha, lo que produce el efecto de estela
void render_clear(RenderContext *ctx, unsigned char r, unsigned char g, unsigned char b);

// Dibuja el arreglo de cuerpos como circulos rellenos usando su color/radio
void render_bodies(RenderContext *ctx, const Body *bodies, int n_bodies);

void render_sun_corona(RenderContext *ctx, const Body *bodies);

void render_explosion_effect(RenderContext *ctx, const Body *bodies, int n_active, float seconds_remaining);

// Presenta el frame
void render_present(RenderContext *ctx);

// Actualiza el FPS mostrado en el titulo de la ventana 
void render_update_fps(RenderContext *ctx);

// Tiempo real transcurrido desde el frame anterior, en segundos (wrapea
// GetFrameTime() de raylib). Pensado para usarse como base de dt en el
// loop de simulacion, sin que main.c necesite conocer raylib.
float render_get_delta_time(RenderContext *ctx);

// Ultimo valor de FPS calculado
double render_get_fps(const RenderContext *ctx);

// Ajusta la intensidad de la estela
void render_set_trail(RenderContext *ctx, unsigned char alpha);

// Utilidad: un canal de color pseudoaleatorio [0-255].
unsigned char rand_color_channel(void);

#endif