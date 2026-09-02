# P1-Screensaver

Simulación N-body (gravedad exacta y aproximada) con estrategias de paralelismo en OpenMP, renderizada como screensaver con raylib.

## Compilar

```bash
make clean
make            # build por defecto: -O3, sin -march=native (portable)
make ARCH_NATIVE=1   # -O3 -march=native, mas rapido pero atado a esta CPU
make omp        # ademas habilita OpenMP en main.o/render.o (no necesario:
                # physics/spatial/corona/barnes_hut/bench ya lo llevan siempre)
```

## Modo interactivo (screensaver)

```bash
./main <N> [modo] [schedule]
# modo:     seq | datos | espacial | tareas   (default: seq)
# schedule: static | dynamic | guided (solo aplica a datos/espacial)

./main 5000 datos static
```

## Modo benchmark (headless)

No abre ventana, no usa raylib, usa semilla y `dt` fijos, y desactiva fusiones/explosiones para mantener `N` constante durante la medición.

```bash
./main --benchmark --bodies 10000 --steps 200 --threads 8 --mode datos --schedule static
```

Flags:

| Flag | Default | Descripción |
|---|---|---|
| `--bodies N` | *(obligatorio)* | número de cuerpos |
| `--mode M` | `datos` | `seq` \| `datos` \| `espacial` \| `newton3` \| `soa` \| `barneshut` \| `tareas` |
| `--steps S` | `100` | pasos medidos |
| `--warmup W` | `5` | pasos de calentamiento (no se miden) |
| `--threads T` | *(default de OpenMP)* | `omp_set_num_threads(T)` |
| `--schedule K` | `static` | `static` \| `dynamic` \| `guided` (solo `datos`/`espacial`) |
| `--theta X` | `0.5` | ángulo de apertura de Barnes-Hut (`0` = gravedad exacta) |
| `--repeat R` | `1` | repite la corrida completa R veces y reporta la mediana |
| `--dt X` | `1.0` | paso de tiempo fijo |

Imprime dos tiempos: el del kernel de fuerzas (incluye construcción de grid/árbol cuando aplica) y el de la física completa (kernel + `update_bodies`).

### Estrategias disponibles

- `seq`: gravedad exacta, secuencial.
- `datos`: gravedad exacta, `parallel for` por cuerpo destino (referencia de paralelismo).
- `espacial`: gravedad exacta, reparto por celda de un grid espacial — **sigue siendo O(N²)**, no reduce interacciones (ver diagnóstico).
- `newton3`: gravedad exacta, explota la 3ª ley de Newton (cada pareja una sola vez) con acumuladores privados por hilo.
- `soa`: gravedad exacta, kernel sobre estructura de arreglos (x/y/mass planos) para mejor localidad de caché.
- `barneshut`: aproximación jerárquica O(N log N) vía quadtree; `--theta` controla la precisión.
- `tareas`: gravedad exacta, `omp task` explícito por bloque de cuerpos (en vez de `omp for`); el runtime reparte los tasks dinámicamente entre hilos.

## Notas de diseño

- `CONST_G` es un literal `float` (sufijo `f`); mezclarlo con `double` promovía parte del kernel a doble precisión.
- El modo `datos` en el loop interactivo usa `simulate_step_datos()`, que calcula fuerzas y actualiza posiciones en una sola región `#pragma omp parallel` (evita abrir dos regiones por frame).
- La corona solar se genera enteramente en un fragment shader (`CORONA_FS` en `render_shaders.h`); ya no hay generación por CPU ni `UpdateTexture()` por frame. `corona.c`/`corona.h` se conservan como referencia de la versión CPU (ya no se usan desde `render.c`) y son útiles para comparar tiempos de generación CPU vs. GPU si se desea.
