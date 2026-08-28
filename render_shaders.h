#ifndef RENDER_SHADERS_H
#define RENDER_SHADERS_H

// Fondo estrellado procedural donde cada celda de una grilla invisible tiene
// una probabilidad baja de contener una estrella, en una posicion y brillo
// pseudoaleatorios fijos por celda (hash de sus coordenadas), con un
// parpadeo suave animado por u_time.
static const char *STAR_FIELD_FS =
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec4 colDiffuse;\n"
"uniform vec2 u_resolution;\n"
"uniform float u_time;\n"
"uniform float u_starDensity;\n"
"uniform float u_twinkleSpeed;\n"
"out vec4 finalColor;\n"
"float hash(vec2 p) {\n"
"  p = fract(p * vec2(123.34, 456.21));\n"
"  p += dot(p, p + 45.32);\n"
"  return fract(p.x * p.y);\n"
"}\n"
"void main() {\n"
"  vec2 pixel = fragTexCoord * u_resolution;\n"
"  float cellSize = 6.0;\n"
"  vec2 cell = floor(pixel / cellSize);\n"
"  float h = hash(cell);\n"
"  if (h > u_starDensity) { discard; }\n"
"  vec2 starLocalPos = fract(pixel / cellSize) - vec2(hash(cell + 1.0), hash(cell + 2.0));\n"
"  float d = length(starLocalPos);\n"
"  float brightness = 0.5 + 0.5 * hash(cell + 3.0);\n"
"  float twinkle = 0.5 + 0.5 * sin(u_time * u_twinkleSpeed + h * 100.0);\n"
"  float intensity = smoothstep(0.35, 0.0, d) * brightness * (0.4 + 0.6 * twinkle);\n"
"  finalColor = vec4(vec3(1.0), intensity) * colDiffuse;\n"
"}\n";

// Post-procesado de pantalla completa para eñ contraste alrededor del gris medio,
// ajuste de saturacion, y una vineta suave hacia los bordes. Se aplica
// sobre la escena ya renderizada, asi que se ve por
// igual sobre el fondo, las estelas, los cuerpos y sus sombras.
static const char *POSTPROCESS_FS =
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec4 colDiffuse;\n"
"uniform float u_contrast;\n"
"uniform float u_saturation;\n"
"uniform float u_vignetteStrength;\n"
"out vec4 finalColor;\n"
"void main() {\n"
"  vec3 color = texture(texture0, fragTexCoord).rgb;\n"
"  color = (color - 0.5) * u_contrast + 0.5;\n"
"  float gray = dot(color, vec3(0.299, 0.587, 0.114));\n"
"  color = mix(vec3(gray), color, u_saturation);\n"
"  vec2 uv = fragTexCoord - 0.5;\n"
"  float vig = 1.0 - u_vignetteStrength * dot(uv, uv) * 2.0;\n"
"  color *= clamp(vig, 0.0, 1.0);\n"
"  finalColor = vec4(clamp(color, 0.0, 1.0), 1.0) * colDiffuse;\n"
"}\n";

#endif