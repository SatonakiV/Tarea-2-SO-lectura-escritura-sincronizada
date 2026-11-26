"""Gráficos de E2 a partir de promedios.csv (requiere matplotlib).

Un archivo por métrica graficada (tiempos y esperas; los fallos atómicos van
en tabla): un panel por caso con su propia escala, porque el caso 4 (K=10000)
tiene otra magnitud. Barras = promedio de las ejecuciones válidas, línea = ±
desviación. Un grupo sin ejecuciones válidas se marca NA.
"""
import argparse
import sys
from pathlib import Path

from comun import leer_csv

COLORES = {"cond": "#2a78d6", "atomic": "#eb6834"}  # paleta categórica validada (slots 1 y 2)
ETIQUETAS = {"cond": "cond escritores", "atomic": "atomic escritores"}
TEXTO, SECUNDARIO, REJILLA = "#0b0b0b", "#52514e", "#e4e3df"
METRICAS = [
    ("pared_s", "Tiempo de pared (s)"),
    ("cpu_s", "Tiempo de CPU del proceso (s)"),
    ("espera_escritura_media_us", "Espera media de escritoras (µs)"),
    ("espera_lectura_media_us", "Espera media de lectoras (µs)"),
    ("espera_lectura_max_us", "Espera máxima de lectoras (µs, promedio de los máximos por ejecución)"),
    ("espera_escritura_max_us", "Espera máxima de escritoras (µs, promedio de los máximos por ejecución)"),
]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--entrada", type=Path, default=Path("resultados/e2/promedios.csv"))
    p.add_argument("--salida", type=Path, default=Path("resultados/e2/graficos"))
    a = p.parse_args()
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        sys.exit("Falta matplotlib: python3 -m pip install matplotlib (o apt install python3-matplotlib)")

    filas = leer_csv(a.entrada)
    casos = sorted({int(f["caso"]) for f in filas})
    por = {(int(f["caso"]), f["modo"]): f for f in filas}
    a.salida.mkdir(parents=True, exist_ok=True)
    plt.rcParams.update({"font.size": 9, "text.color": TEXTO, "axes.labelcolor": SECUNDARIO,
                         "xtick.color": SECUNDARIO, "ytick.color": SECUNDARIO})

    for metrica, titulo in METRICAS:
        fig, ejes = plt.subplots(1, len(casos), figsize=(2.1 * len(casos), 2.8))
        for eje, caso in zip(ejes, casos):
            g0 = por.get((caso, "cond")) or por.get((caso, "atomic"))
            for i, modo in enumerate(("cond", "atomic")):
                f = por.get((caso, modo))
                if not f or not f.get(f"{metrica}_media"):
                    # Grupo sin ejecuciones válidas: se marca en vez de dibujar un valor.
                    eje.annotate("NA", (i, 0), xytext=(0, 3), textcoords="offset points",
                                 ha="center", fontsize=8, color=SECUNDARIO)
                    continue
                m, d = float(f[f"{metrica}_media"]), float(f[f"{metrica}_desv"])
                eje.bar(i, m, width=0.6, color=COLORES[modo], edgecolor="white",
                        linewidth=2, label=ETIQUETAS[modo], zorder=2)
                eje.errorbar(i, m, yerr=d, color=SECUNDARIO, linewidth=1, capsize=3, zorder=3)
                etiqueta = f"{m:,.0f}".replace(",", ".") if m >= 100 else f"{m:.3g}"
                eje.annotate(etiqueta, (i, m + d), xytext=(0, 3), textcoords="offset points",
                             ha="center", fontsize=8, color=TEXTO)
            eje.set_title(f"Caso {caso}\nL={g0['L']} E={g0['E']} K={g0['K']}\n"
                          f"{g0['dur_lec_us']}/{g0['dur_esc_us']} µs", fontsize=8, color=TEXTO)
            eje.set_xticks([])
            eje.margins(y=0.18)
            eje.grid(axis="y", color=REJILLA, linewidth=0.8, zorder=0)
            for lado in ("top", "right", "bottom"):
                eje.spines[lado].set_visible(False)
            eje.spines["left"].set_color(REJILLA)
            eje.set_ylim(bottom=0)
        manejadores, nombres = ejes[0].get_legend_handles_labels()
        fig.legend(manejadores, nombres, loc="upper center", ncol=2, frameon=False,
                   bbox_to_anchor=(0.5, 1.0))
        fig.suptitle(titulo, y=0.9, fontsize=10, color=TEXTO, x=0.01, ha="left")
        fig.tight_layout(rect=(0, 0, 1, 0.86))
        destino = a.salida / f"{metrica}.png"
        fig.savefig(destino, dpi=200)
        plt.close(fig)
        print(f"Gráfico: {destino}")


if __name__ == "__main__":
    main()
