#ifndef BENCH_H
#define BENCH_H

// Modo benchmark headless: no inicializa raylib, no abre ventana, usa
// semilla y dt fijos, numero de pasos fijo, y desactiva fusiones y
// explosiones durante la medicion (ver seccion 10 del plan de
// optimizacion). Se invoca como:
//
//   ./main --benchmark --bodies 10000 --steps 200 --threads 8 --mode datos
//
// Flags soportadas:
//   --bodies N       (obligatorio) numero de cuerpos, 2 <= N <= MAX_BODIES
//   --mode M         seq | datos | espacial | tareas (default: datos)
//   --steps S        pasos medidos (default: 100)
//   --warmup W       pasos de calentamiento antes de medir (default: 5)
//   --threads T      omp_set_num_threads(T); si se omite, usa el default de OpenMP
//   --schedule K     static | dynamic | guided (solo aplica a datos/espacial)
//   --repeat R       repite la medicion R veces y reporta la mediana (default: 1)
//   --dt X           paso de tiempo fijo (default: 1.0)
//
// Devuelve el codigo de salida del proceso (0 en exito).
int run_benchmark(int argc, char *argv[]);

#endif