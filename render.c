#include <math.h>
#include <stdlib.h>
#include "raylib.h"
#include "render.h"
#include "render_shaders.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEFAULT_TRAIL_ALPHA 150


#define SHADOW_DARKEN_FACTOR 0.45f
#define SHADOW_MAX_ALPHA 130
#define SHADOW_SEGMENTS 24

#define SHADOW_RADIUS_OVERSHOOT 1.08f
#define SHADOW_FEATHER_STEPS 2
#define SHADOW_FEATHER_DEG 50.0f

#define CORONA_SCREEN_SCALE 4.5f


#define STAR_DENSITY 0.05f
#define STAR_TWINKLE_SPEED 2.2f
#define POST_CONTRAST 1.18f
#define POST_SATURATION 1.12f
#define POST_VIGNETTE_STRENGTH 0.35f

struct RenderShaderState {
  RenderTexture2D scene;
  Shader star_shader;
  int loc_star_time;
  Shader post_shader;

  // Corona generada en GPU (ver CORONA_FS en render_shaders.h): reemplaza
  // el enfoque anterior de generar una textura por frame en CPU y subirla
  // con UpdateTexture(), que agregaba una transferencia CPU->GPU y una
  // region OpenMP extra cada frame solo para el efecto visual.
  Shader corona_shader;
  int loc_corona_time;
  int loc_corona_center;
  int loc_corona_radius;
};

static RenderShaderState *shader_state_init(int width, int height) {
  RenderShaderState *st = malloc(sizeof(RenderShaderState));
  if (st == NULL) return NULL;

  st->scene = LoadRenderTexture(width, height);
  st->star_shader = LoadShaderFromMemory(NULL, STAR_FIELD_FS);
  st->post_shader = LoadShaderFromMemory(NULL, POSTPROCESS_FS);
  st->corona_shader = LoadShaderFromMemory(NULL, CORONA_FS);

  if (st->scene.id == 0 || st->star_shader.id == 0 || st->post_shader.id == 0 || st->corona_shader.id == 0) {
    TraceLog(LOG_WARNING, "No se pudieron inicializar los shaders; se sigue sin ellos.");
    if (st->scene.id != 0) UnloadRenderTexture(st->scene);
    if (st->star_shader.id != 0) UnloadShader(st->star_shader);
    if (st->post_shader.id != 0) UnloadShader(st->post_shader);
    if (st->corona_shader.id != 0) UnloadShader(st->corona_shader);
    free(st);
    return NULL;
  }

  st->loc_star_time = GetShaderLocation(st->star_shader, "u_time");
  st->loc_corona_time = GetShaderLocation(st->corona_shader, "u_time");
  st->loc_corona_center = GetShaderLocation(st->corona_shader, "u_center");
  st->loc_corona_radius = GetShaderLocation(st->corona_shader, "u_radius");
  int loc_star_res  = GetShaderLocation(st->star_shader, "u_resolution");
  int loc_star_dens = GetShaderLocation(st->star_shader, "u_starDensity");
  int loc_star_twk  = GetShaderLocation(st->star_shader, "u_twinkleSpeed");

  Vector2 resolution = { (float)width, (float)height };
  float density = STAR_DENSITY;
  float twinkle = STAR_TWINKLE_SPEED;
  SetShaderValue(st->star_shader, loc_star_res, &resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(st->star_shader, loc_star_dens, &density, SHADER_UNIFORM_FLOAT);
  SetShaderValue(st->star_shader, loc_star_twk, &twinkle, SHADER_UNIFORM_FLOAT);

  int loc_contrast  = GetShaderLocation(st->post_shader, "u_contrast");
  int loc_saturation = GetShaderLocation(st->post_shader, "u_saturation");
  int loc_vignette  = GetShaderLocation(st->post_shader, "u_vignetteStrength");
  float contrast = POST_CONTRAST;
  float saturation = POST_SATURATION;
  float vignette = POST_VIGNETTE_STRENGTH;
  SetShaderValue(st->post_shader, loc_contrast, &contrast, SHADER_UNIFORM_FLOAT);
  SetShaderValue(st->post_shader, loc_saturation, &saturation, SHADER_UNIFORM_FLOAT);
  SetShaderValue(st->post_shader, loc_vignette, &vignette, SHADER_UNIFORM_FLOAT);

  return st;
}

static void shader_state_free(RenderShaderState *st) {
  if (st == NULL) return;
  UnloadRenderTexture(st->scene);
  UnloadShader(st->star_shader);
  UnloadShader(st->post_shader);
  UnloadShader(st->corona_shader);
  free(st);
}

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

  ctx->shaders = shader_state_init(width, height);

  return 1;
}

void render_shutdown(RenderContext *ctx) {
  shader_state_free(ctx->shaders);
  ctx->shaders = NULL;
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

  if (ctx->shaders != NULL) {
    BeginTextureMode(ctx->shaders->scene);
  } else {
    BeginDrawing();
  }

  Color bg = (Color){ r, g, b, ctx->trail_alpha };
  DrawRectangle(0, 0, ctx->width, ctx->height, bg);

  if (ctx->shaders != NULL) {
    float t = (float)GetTime();
    SetShaderValue(ctx->shaders->star_shader, ctx->shaders->loc_star_time, &t, SHADER_UNIFORM_FLOAT);

    BeginShaderMode(ctx->shaders->star_shader);
    DrawRectangle(0, 0, ctx->width, ctx->height, WHITE);
    EndShaderMode();
  }
}

static Color darken_color(Color c, float factor) {
  return (Color){
    (unsigned char)(c.r * factor),
    (unsigned char)(c.g * factor),
    (unsigned char)(c.b * factor),
    c.a
  };
}

#define CORONA_SEGMENTS 64

void render_sun_corona(RenderContext *ctx, const Body *bodies) {
  if (ctx->shaders == NULL) return;

  float t = (float)GetTime();
  SetShaderValue(ctx->shaders->corona_shader, ctx->shaders->loc_corona_time, &t, SHADER_UNIFORM_FLOAT);

  const Body *sun = &bodies[0];
  float corona_size = sun->radius * CORONA_SCREEN_SCALE;
  Vector2 center = { sun->x, sun->y };
  float radius = corona_size / 2.0f;

  SetShaderValue(ctx->shaders->corona_shader, ctx->shaders->loc_corona_center, &center, SHADER_UNIFORM_VEC2);
  SetShaderValue(ctx->shaders->corona_shader, ctx->shaders->loc_corona_radius, &radius, SHADER_UNIFORM_FLOAT);

  BeginBlendMode(BLEND_ADDITIVE);
  BeginShaderMode(ctx->shaders->corona_shader);
  DrawCircleSector(center, radius, 0, 360, CORONA_SEGMENTS, WHITE);
  EndShaderMode();
  EndBlendMode();
}

void render_bodies(RenderContext *ctx, const Body *bodies, int n_bodies) {
  (void)ctx;
  const Body *sun = &bodies[0];

  int draw_shadows = n_bodies <= 5000;

  for (int i = 0; i < n_bodies; i++) {
    const Body *body = &bodies[i];
    Color color = (Color){ body->r, body->g, body->b, 255 };
    DrawCircle((int)roundf(body->x), (int)roundf(body->y), body->radius, color);

    if (i == 0) continue; 

    float dx = body->x - sun->x;
    float dy = body->y - sun->y;
    if (dx == 0.0f && dy == 0.0f) continue;

    if (!draw_shadows) continue;

    float away_from_sun_deg = atan2f(dy, dx) * (180.0f / (float)M_PI);

    Color shadow_base = darken_color(color, SHADOW_DARKEN_FACTOR);
    Vector2 center = { body->x, body->y };
    float shadow_radius = body->radius * SHADOW_RADIUS_OVERSHOOT;

    for (int step = 0; step < SHADOW_FEATHER_STEPS; step++) {
      float t = (float)(step + 1) / (float)SHADOW_FEATHER_STEPS;
      float half_span = 90.0f - SHADOW_FEATHER_DEG * (1.0f - t);
      Color shadow = shadow_base;
      shadow.a = (unsigned char)(SHADOW_MAX_ALPHA * t);
      DrawCircleSector(center, shadow_radius, away_from_sun_deg - half_span, away_from_sun_deg + half_span, SHADOW_SEGMENTS, shadow);
    }
  }
}

void render_explosion_effect(RenderContext *ctx, const Body *bodies, int n_active, float seconds_remaining) {
  (void)n_active;

  float progress = 1.0f - (seconds_remaining / EXPLOSION_DURATION_SECONDS);
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
  if (ctx->shaders != NULL) {
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);

    BeginShaderMode(ctx->shaders->post_shader);

    Rectangle src = { 0, 0, (float)ctx->shaders->scene.texture.width, -(float)ctx->shaders->scene.texture.height };
    DrawTextureRec(ctx->shaders->scene.texture, src, (Vector2){ 0, 0 }, WHITE);
    EndShaderMode();
  }

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

float render_get_delta_time(RenderContext *ctx) {
  (void)ctx;
  return GetFrameTime();
}

void render_set_trail(RenderContext *ctx, unsigned char alpha) {
  ctx->trail_alpha = alpha;
}

unsigned char rand_color_channel(void) {
  return (unsigned char)GetRandomValue(0, 255);
}