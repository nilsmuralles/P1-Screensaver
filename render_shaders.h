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
"  vec2 pixel = gl_FragCoord.xy;\n"
"  float cellSize = 4.0;\n"
"  vec2 cell = floor(pixel / cellSize);\n"
"  float h = hash(cell);\n"
"  if (h > u_starDensity) { discard; }\n"
"  vec2 starLocalPos = fract(pixel / cellSize) - vec2(hash(cell + 1.0), hash(cell + 2.0));\n"
"  float d = length(starLocalPos);\n"
"  float brightness = 0.65 + 0.35 * hash(cell + 3.0);\n"
"  float twinkle = 0.5 + 0.5 * sin(u_time * u_twinkleSpeed + h * 100.0);\n"
"  float intensity = smoothstep(0.6, 0.0, d) * brightness * (0.55 + 0.45 * twinkle);\n"
"  finalColor = vec4(vec3(1.0), intensity) * colDiffuse;\n"
"}\n";

// Corona solar generada enteramente en el fragment shader (equivalente a
// corona_generate() en corona.c, pero sin CPU: sin generar una textura por
// frame ni transferirla a la GPU con UpdateTexture). Se dibuja sobre un
// rectangulo del tamano/posicion de la corona (ver render_sun_corona en
// render.c), asi que fragTexCoord ya llega normalizado en [0,1] dentro de
// ese rectangulo.
static const char *CORONA_FS =
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec4 colDiffuse;\n"
"uniform float u_time;\n"
"uniform vec2 u_center;\n"
"uniform float u_radius;\n"
"out vec4 finalColor;\n"
// "noise2" es un nombre reservado en GLSL (funcion de ruido Perlin de
// versiones antiguas del lenguaje); algunos compiladores lo siguen
// tratando como predeclarado y rompen la compilacion si se redefine con
// otra firma. Se renombra a "corona_noise" para evitar el choque.
"float corona_noise(vec2 p) {\n"
"  float h = sin(p.x * 127.1 + p.y * 311.7) * 43758.5453;\n"
"  return fract(h);\n"
"}\n"
"void main() {\n"
// gl_FragCoord (posicion real en pixeles) en vez de fragTexCoord: igual que
// con el fondo de estrellas, DrawCircleSector usa la textura interna de
// "figuras" de raylib y no garantiza un fragTexCoord mapeado 0..1 sobre la
// forma dibujada.
"  vec2 centered = (gl_FragCoord.xy - u_center) / u_radius;\n"
"  float dist = length(centered);\n"
// Descarta explicitamente los fragmentos fuera del circulo unitario: no
// basta con que baje el alpha a 0, porque si el blend mode no respeta el
// canal alpha del fragmento (segun la implementacion), el rectangulo
// completo queda visible como un cuadrado solido alrededor del sol.
"  if (dist > 1.0) {\n"
"    discard;\n"
"  }\n"
"  float angle = atan(centered.y, centered.x);\n"
"  float flicker = 0.5 + 0.5 * sin(angle * 6.0 + u_time * 1.5 + corona_noise(gl_FragCoord.xy) * 3.0);\n"
"  float falloff = 1.0 - dist;\n"
"  falloff = falloff * falloff;\n"
"  float intensity = clamp(falloff * (0.6 + 0.4 * flicker), 0.0, 1.0);\n"
"  vec3 color = vec3(1.0, (200.0 + 55.0 * intensity) / 255.0, (70.0 + 50.0 * intensity) / 255.0);\n"
// Alpha premultiplicado como segunda salvaguarda contra el mismo problema.
"  finalColor = vec4(color * intensity, intensity) * colDiffuse;\n"
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