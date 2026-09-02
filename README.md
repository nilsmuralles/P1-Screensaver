# P1-Screensaver

Simulación N-body (gravedad exacta) con estrategias de paralelismo en OpenMP, renderizada como screensaver con raylib. Todas las estrategias calculan el mismo trabajo O(N²) por paso — solo cambia cómo se reparte entre hilos — para que la comparación de speedup/eficiencia entre ellas sea justa (ninguna gana por hacer menos trabajo algorítmico).

## Compilar

```bash
make clean
make            # build por defecto: -O3, sin -march=native (portable)
make ARCH_NATIVE=1   # -O3 -march=native, mas rapido pero atado a esta CPU
make omp        # ademas habilita OpenMP en main.o/render.o (no necesario:
                # physics/spatial/corona/bench ya lo llevan siempre)
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
| `--mode M` | `datos` | `seq` \| `datos` \| `espacial` \| `tareas` |
| `--steps S` | `100` | pasos medidos |
| `--warmup W` | `5` | pasos de calentamiento (no se miden) |
| `--threads T` | *(default de OpenMP)* | `omp_set_num_threads(T)` |
| `--schedule K` | `static` | `static` \| `dynamic` \| `guided` (solo `datos`/`espacial`) |
| `--repeat R` | `1` | repite la corrida completa R veces y reporta la mediana |
| `--dt X` | `1.0` | paso de tiempo fijo |

Imprime dos tiempos: el del kernel de fuerzas (incluye construcción de grid cuando aplica) y el de la física completa (kernel + `update_bodies`).

### Estrategias disponibles

Todas calculan gravedad exacta con el mismo O(N²) de trabajo total por paso; solo difieren en cómo reparten ese trabajo entre hilos, para que el speedup medido refleje la estrategia de paralelización y no una diferencia de complejidad algorítmica.

- `seq`: secuencial, sin OpenMP (referencia/baseline).
- `datos`: `parallel for` por cuerpo destino (paralelismo de datos puro).
- `espacial`: reparto por celda de un grid espacial — cada cuerpo suma la fuerza de **todos** los demás (no solo de celdas vecinas), así que sigue siendo O(N²) real, exactamente comparable con `datos`/`tareas`. Solo cambia la unidad de reparto entre hilos (celda en vez de índice de cuerpo) y el orden de acceso a memoria (agrupado por celda). *Nota histórica: una versión anterior sumaba solo el vecindario de 9 celdas alrededor de cada cuerpo, lo cual truncaba la gravedad a un radio corto y la reducía a ~O(N) — verificado empíricamente antes de corregirlo (a partir de N≈2000 sumaba menos del 3% de las interacciones totales).*
- `tareas`: `omp task` explícito por bloque de cuerpos (en vez de `omp for`); el runtime reparte los tasks dinámicamente entre hilos.

## Notas de diseño

- `CONST_G` es un literal `float` (sufijo `f`); mezclarlo con `double` promovía parte del kernel a doble precisión.
- El modo `datos` en el loop interactivo usa `simulate_step_datos()`, que calcula fuerzas y actualiza posiciones en una sola región `#pragma omp parallel` (evita abrir dos regiones por frame).
- La corona solar se genera enteramente en un fragment shader (`CORONA_FS` en `render_shaders.h`); ya no hay generación por CPU ni `UpdateTexture()` por frame. `corona.c`/`corona.h` se conservan como referencia de la versión CPU (ya no se usan desde `render.c`) y son útiles para comparar tiempos de generación CPU vs. GPU si se desea.
