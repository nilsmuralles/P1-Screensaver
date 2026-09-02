import pandas as pd
import matplotlib.pyplot as plt
import pathlib

RESULTS_DIR = pathlib.Path("results")
RAW_CSV = RESULTS_DIR / "bench_raw.csv"

# Estrategias con gravedad exacta O(N^2), directamente comparables entre si
# y contra "seq" para calcular speedup/eficiencia real de paralelismo.
COMPARABLE_MODES = ["datos", "tareas"]

# "espacial" solo calcula fuerzas contra los cuerpos de las celdas vecinas
# (3x3), no contra todos los N cuerpos como las demas estrategias. 
# Al hacer menos calculos, cualquier tiempo mas bajo no viene del paralelismo 
# sino de que resuelve un problema mas chico. Comparar su speedup contra "seq" 
# seria enganoso. Por eso se grafica en un panel aparte, solo como referencia.
APPROX_MODES = ["espacial"]

# Nucleos fisicos reales de la maquina de prueba: 4 core / 8
# hilos logicos por Hyperthreading. La curva principal de speedup/
# eficiencia se limita a hilos <= nucleos fisicos. 8 hilos se muestra
# aparte como caso de contencion de HT.
PHYSICAL_CORES = 4
MAIN_THREADS = [1, 2, 4]
EXTRA_THREADS = [8]


def load_medians():
    df = pd.read_csv(RAW_CSV)
    # Para cada combinacion de modo/N/hilos tenemos 10 tiempos medidos.
    # Usamos la mediana en vez del promedio porque
    # el promedio se deja arrastrar por valores extremos. Es el
    # mismo criterio que ya usa bench.c.
    med = df.groupby(["modo", "n_bodies", "hilos"])["total_s"].median().reset_index()
    med = med.rename(columns={"total_s": "mediana_s"})
    return med


def speedup_table(med):
    seq = med[med["modo"] == "seq"].set_index("n_bodies")["mediana_s"]

    rows = []
    for _, row in med.iterrows():
        if row["modo"] == "seq":
            continue
        n = row["n_bodies"]
        if n not in seq.index:
            continue
        baseline = seq.loc[n]
        speedup = baseline / row["mediana_s"]
        hilos = row["hilos"] if row["hilos"] > 0 else 1
        rows.append({
            "modo": row["modo"],
            "n_bodies": n,
            "hilos": hilos,
            "mediana_s": row["mediana_s"],
            "speedup": speedup,
            "eficiencia": speedup / hilos,
        })
    return pd.DataFrame(rows)


def plot_speedup_efficiency(speedup_df):
    n_values = sorted(speedup_df["n_bodies"].unique())
    fig, axes = plt.subplots(1, 2, figsize=(13, 5))

    for mode in COMPARABLE_MODES:
        sub_mode = speedup_df[speedup_df["modo"] == mode]
        for n in n_values:
            sub = sub_mode[(sub_mode["n_bodies"] == n) & (sub_mode["hilos"].isin(MAIN_THREADS))]
            sub = sub.sort_values("hilos")
            if sub.empty:
                continue
            label = f"{mode} N={n}"
            axes[0].plot(sub["hilos"], sub["speedup"], marker="o", label=label)
            axes[1].plot(sub["hilos"], sub["eficiencia"], marker="o", label=label)

    ideal_threads = MAIN_THREADS
    axes[0].plot(ideal_threads, ideal_threads, "k--", alpha=0.4, label="speedup ideal")
    axes[0].set_xlabel("Hilos")
    axes[0].set_ylabel("Speedup (T_seq / T_paralelo)")
    axes[0].set_title(f"Speedup (<= {PHYSICAL_CORES} nucleos fisicos)")
    axes[0].set_xticks(MAIN_THREADS)
    axes[0].legend(fontsize=7)
    axes[0].grid(alpha=0.3)

    axes[1].axhline(1.0, color="k", linestyle="--", alpha=0.4, label="eficiencia ideal")
    axes[1].set_xlabel("Hilos")
    axes[1].set_ylabel("Eficiencia (speedup / hilos)")
    axes[1].set_title("Eficiencia (<= 4 nucleos fisicos)")
    axes[1].set_xticks(MAIN_THREADS)
    axes[1].set_ylim(0, 1.15)
    axes[1].legend(fontsize=7)
    axes[1].grid(alpha=0.3)

    fig.suptitle("Estrategias por datos y por tareas vs. secuencial (gravedad exacta)")
    fig.tight_layout()
    out = RESULTS_DIR / "speedup_efficiency.png"
    fig.savefig(out, dpi=150)
    print(f"Guardado: {out}")


def plot_hyperthreading_case(speedup_df):
    # Caso aparte: que pasa en 8 hilos (2 por nucleo fisico via HT) frente
    # a 4 hilos (1 por nucleo). No es parte de la curva "ideal" de arriba.
    n_values = sorted(speedup_df["n_bodies"].unique())
    fig, ax = plt.subplots(figsize=(7, 5))

    for mode in COMPARABLE_MODES:
        sub_mode = speedup_df[speedup_df["modo"] == mode]
        for n in n_values:
            sub = sub_mode[(sub_mode["n_bodies"] == n) & (sub_mode["hilos"].isin([4, 8]))]
            sub = sub.sort_values("hilos")
            if len(sub) < 2:
                continue
            ax.plot(sub["hilos"], sub["eficiencia"], marker="o", label=f"{mode} N={n}")

    ax.axvline(PHYSICAL_CORES, color="r", linestyle=":", alpha=0.6,
               label=f"{PHYSICAL_CORES} nucleos fisicos")
    ax.set_xlabel("Hilos")
    ax.set_ylabel("Eficiencia (speedup / hilos)")
    ax.set_xticks([4, 8])
    ax.set_title("Caida de eficiencia al superar nucleos fisicos (Hyperthreading)")
    ax.legend(fontsize=8)
    ax.grid(alpha=0.3)
    fig.tight_layout()
    out = RESULTS_DIR / "hyperthreading_8_hilos.png"
    fig.savefig(out, dpi=150)
    print(f"Guardado: {out}")


def plot_espacial_reference(med):
    n_values = sorted(med["n_bodies"].unique())
    fig, ax = plt.subplots(figsize=(7, 5))

    for mode in ["seq"] + COMPARABLE_MODES + APPROX_MODES:
        sub_mode = med[med["modo"] == mode]
        xs, ys = [], []
        for n in n_values:
            row = sub_mode[sub_mode["n_bodies"] == n]
            if mode == "seq":
                r = row
            else:
                r = row[row["hilos"] == 4]
            if r.empty:
                continue
            xs.append(n)
            ys.append(r["mediana_s"].values[0])
        style = "--" if mode in APPROX_MODES else "-"
        ax.plot(xs, ys, style, marker="o", label=f"{mode}" + (" (4 hilos)" if mode != "seq" else ""))

    ax.set_xlabel("N (cuerpos)")
    ax.set_ylabel("Tiempo mediana (s), fisica total")
    ax.set_yscale("log")
    ax.set_title("Espacial vs. Estrategias exactas\n")
    ax.legend(fontsize=8)
    ax.grid(alpha=0.3, which="both")
    fig.tight_layout()
    out = RESULTS_DIR / "espacial_referencia.png"
    fig.savefig(out, dpi=150)
    print(f"Guardado: {out}")


def main():
    RESULTS_DIR.mkdir(exist_ok=True)
    med = load_medians()
    speedup_df = speedup_table(med)

    speedup_df.to_csv(RESULTS_DIR / "speedup_efficiency_table.csv", index=False)
    print(f"Guardado: {RESULTS_DIR / 'speedup_efficiency_table.csv'}")

    plot_speedup_efficiency(speedup_df)
    plot_hyperthreading_case(speedup_df)
    plot_espacial_reference(med)


if __name__ == "__main__":
    main()
