# Corre las 4 configuraciones (secuencial + los 3 modos paralelos) variando
# N y numero de hilos

set -euo pipefail

BODIES_LIST=(500 2000 5000 10000)
THREADS_LIST=(1 2 4 8)
MODES=(datos espacial tareas) 
STEPS=100
WARMUP=5
REPEATS=10 
BINARY=./main
OUT_DIR=results
OUT_CSV="$OUT_DIR/bench_raw.csv"

mkdir -p "$OUT_DIR"

if [ ! -x "$BINARY" ]; then
  echo "Error: no se encontro '$BINARY' ejecutable. Corre 'make omp' primero." >&2
  exit 1
fi

echo "modo,n_bodies,hilos,repeticion,kernel_s,total_s" > "$OUT_CSV"

run_once() {
  local mode=$1 n=$2 threads=$3 rep=$4
  local args=(--benchmark --bodies "$n" --mode "$mode" --steps "$STEPS" --warmup "$WARMUP" --repeat 1)
  if [ "$threads" -gt 0 ]; then
    args+=(--threads "$threads")
  fi

  local out
  out=$("$BINARY" "${args[@]}")

  local kernel total
  kernel=$(echo "$out" | grep '^kernel:' | grep -oE 'mediana=[0-9.]+' | cut -d= -f2)
  total=$(echo "$out"  | grep '^fisica_total:' | grep -oE 'mediana=[0-9.]+' | cut -d= -f2)

  if [ -z "$kernel" ] || [ -z "$total" ]; then
    echo "Aviso: no se pudo parsear salida para modo=$mode N=$n hilos=$threads rep=$rep" >&2
    echo "$out" >&2
    return
  fi

  echo "$mode,$n,$threads,$rep,$kernel,$total" >> "$OUT_CSV"
}

echo "=== Barrido: secuencial (baseline, sin variar hilos) ==="
for n in "${BODIES_LIST[@]}"; do
  for rep in $(seq 1 "$REPEATS"); do
    echo "  seq N=$n rep=$rep/$REPEATS"
    run_once seq "$n" 0 "$rep"
  done
done

echo "=== Barrido: modos paralelos (${MODES[*]}) x hilos (${THREADS_LIST[*]}) ==="
for mode in "${MODES[@]}"; do
  for n in "${BODIES_LIST[@]}"; do
    for threads in "${THREADS_LIST[@]}"; do
      for rep in $(seq 1 "$REPEATS"); do
        echo "  modo=$mode N=$n hilos=$threads rep=$rep/$REPEATS"
        run_once "$mode" "$n" "$threads" "$rep"
      done
    done
  done
done

echo "Listo. Resultados crudos en $OUT_CSV"
