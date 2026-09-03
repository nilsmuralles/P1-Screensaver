# Compara static/dynamic/guided para datos y espacial, solo en N=10000
# (donde mas se nota la diferencia), hilos 2/4/8. El caso static a estos
# mismos N/hilos ya esta en results/bench_raw.csv (bench_sweep.sh usa
# static por defecto), asi que aqui solo se corre dynamic y guided.

set -euo pipefail

N=10000
THREADS_LIST=(2 4 8)
MODES=(datos espacial)
SCHEDULES=(dynamic guided)
STEPS=100
WARMUP=5
REPEATS=10
BINARY=./main
OUT_CSV=results/bench_schedule.csv

if [ ! -x "$BINARY" ]; then
  echo "Error: no se encontro '$BINARY' ejecutable. Corre 'make' primero." >&2
  exit 1
fi

echo "modo,n_bodies,hilos,schedule,repeticion,kernel_s,total_s" > "$OUT_CSV"

for mode in "${MODES[@]}"; do
  for threads in "${THREADS_LIST[@]}"; do
    for sched in "${SCHEDULES[@]}"; do
      for rep in $(seq 1 "$REPEATS"); do
        echo "  modo=$mode hilos=$threads schedule=$sched rep=$rep/$REPEATS"
        out=$("$BINARY" --benchmark --bodies "$N" --mode "$mode" --steps "$STEPS" \
              --warmup "$WARMUP" --repeat 1 --threads "$threads" --schedule "$sched")
        kernel=$(echo "$out" | grep '^kernel:' | grep -oE 'mediana=[0-9.]+' | cut -d= -f2)
        total=$(echo "$out"  | grep '^fisica_total:' | grep -oE 'mediana=[0-9.]+' | cut -d= -f2)
        if [ -z "$kernel" ] || [ -z "$total" ]; then
          echo "Aviso: no se pudo parsear salida para modo=$mode hilos=$threads schedule=$sched rep=$rep" >&2
          echo "$out" >&2
          continue
        fi
        echo "$mode,$N,$threads,$sched,$rep,$kernel,$total" >> "$OUT_CSV"
      done
    done
  done
done

echo "Listo. Resultados en $OUT_CSV"
