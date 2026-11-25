"""E2: costo de sincronización.

cond escritores frente a atomic escritores, 5 casos x 5 repeticiones = 50
ejecuciones con notraza. En el caso sin pausas (L=E=8) se usa K=10000.
Genera:
  <salida>/crudo/*.txt   salida original de cada ejecución
  <salida>/e2.csv        una fila por ejecución (datos originales)
  <salida>/promedios.csv promedio y desviación de las repeticiones
  <salida>/resumen.md    tablas para el informe
  <salida>/entorno.txt   máquina, compilador y commit
Termina con código 1 si alguna ejecución falla: esos resultados no deben
usarse como definitivos.
"""
import argparse
from pathlib import Path

from comun import (
    CASO_SIN_PAUSAS, CASOS, K_E2_SIN_PAUSAS, VARIANTES_E2, analizar_salida,
    comando_rw, desviacion, ejecutar, escribir_csv, guardar_crudo, media,
    registrar_entorno,
)

METRICAS = [
    "pared_s", "cpu_s", "espera_lectura_media_us", "espera_lectura_max_us",
    "espera_escritura_media_us", "espera_escritura_max_us", "max_lectoras",
    "fallos_recurso_ocupado", "fallos_escritoras_esperando", "fallos_cas",
]
CAMPOS = [
    "caso", "L", "E", "K", "dur_lec_us", "dur_esc_us", "modo", "politica",
    "repeticion", "codigo", "r2_ok", "correcto", "traza", "solicitudes_lectura",
    "solicitudes_escritura", *METRICAS, "cpu_por_pared", "fallos_saturados",
    "archivo",
]


def casos_e2():
    for n, (L, E, K, dl, de) in enumerate(CASOS, 1):
        yield n, L, E, (K_E2_SIN_PAUSAS if n == CASO_SIN_PAUSAS else K), dl, de


def valida(fila):
    return (fila["codigo"] == 0 and fila.get("r2_ok") == "SI"
            and fila.get("correcto") == "SI" and fila.get("traza") == "desactivada")


def promediar(filas):
    grupos = {}
    for f in filas:
        grupos.setdefault((f["caso"], f["modo"]), []).append(f)
    salida = []
    for (caso, modo), grupo in sorted(grupos.items()):
        g0 = grupo[0]
        fila = {k: g0[k] for k in ("caso", "L", "E", "K", "dur_lec_us", "dur_esc_us", "modo", "politica")}
        fila["repeticiones"] = len(grupo)
        fila["validas"] = sum(valida(f) for f in grupo)
        for m in [*METRICAS, "cpu_por_pared"]:
            valores = [float(f[m]) for f in grupo if f.get(m) not in (None, "")]
            fila[f"{m}_media"] = media(valores)
            fila[f"{m}_desv"] = desviacion(valores)
        salida.append(fila)
    return salida


def _fmt(v, dec=3):
    return f"{v:.{dec}f}"


def resumen(prom, entorno):
    por = {(p["caso"], p["modo"]): p for p in prom}
    lineas = [
        "# E2: costo de sincronización",
        "",
        f"Commit `{entorno['commit']}` (rama `{entorno['rama']}`), {entorno['fecha']}, "
        f"{entorno['cpu']} ({entorno['cpus_logicas']} CPU lógicas), {entorno['compilador']}.",
        "",
        "Promedio de las repeticiones (± desviación estándar), `notraza`. "
        "Pared: `steady_clock`; CPU: `CLOCK_PROCESS_CPUTIME_ID`. Ambas cubren desde "
        "antes de crear las hebras hasta completar todos los `join()`. "
        "El tiempo de CPU incluye trabajo útil, no solo espera.",
        "",
        "## Tiempos",
        "",
        "| Caso | L | E | K | lec µs | esc µs | Variante | Válidas | Pared (s) | CPU (s) | CPU/pared |",
        "|---|---|---|---|---|---|---|---|---|---|---|",
    ]
    for p in prom:
        lineas.append(
            f"| {p['caso']} | {p['L']} | {p['E']} | {p['K']} | {p['dur_lec_us']} | "
            f"{p['dur_esc_us']} | {p['modo']} | {p['validas']}/{p['repeticiones']} | "
            f"{_fmt(p['pared_s_media'], 4)} ± {_fmt(p['pared_s_desv'], 4)} | "
            f"{_fmt(p['cpu_s_media'], 4)} ± {_fmt(p['cpu_s_desv'], 4)} | "
            f"{_fmt(p['cpu_por_pared_media'], 2)} |"
        )
    lineas += [
        "",
        "## Esperas por solicitud (µs, desde antes de `entrar_*()` hasta su retorno)",
        "",
        "| Caso | Variante | Lectura media | Lectura máx. | Escritura media | Escritura máx. | Máx. lectoras |",
        "|---|---|---|---|---|---|---|",
    ]
    for p in prom:
        lineas.append(
            f"| {p['caso']} | {p['modo']} | {_fmt(p['espera_lectura_media_us_media'], 1)} | "
            f"{_fmt(p['espera_lectura_max_us_media'], 1)} | "
            f"{_fmt(p['espera_escritura_media_us_media'], 1)} | "
            f"{_fmt(p['espera_escritura_max_us_media'], 1)} | "
            f"{_fmt(p['max_lectoras_media'], 1)} |"
        )
    lineas += [
        "",
        "## Fallos de admisión del modo atómico (promedio por ejecución)",
        "",
        "| Caso | Recurso ocupado | Escritoras esperando | CAS fallido |",
        "|---|---|---|---|",
    ]
    for p in prom:
        if p["modo"] == "atomic":
            lineas.append(
                f"| {p['caso']} | {p['fallos_recurso_ocupado_media']:.0f} | "
                f"{p['fallos_escritoras_esperando_media']:.0f} | {p['fallos_cas_media']:.0f} |"
            )
    lineas += [
        "",
        "## Razón atomic / cond",
        "",
        "Valores mayores que 1 indican que la variante atómica tarda o consume más.",
        "",
        "| Caso | Pared | CPU | Espera escritura media | Espera lectura media |",
        "|---|---|---|---|---|",
    ]

    def razon(caso, m):
        c, a = por.get((caso, "cond")), por.get((caso, "atomic"))
        if not c or not a or not c[f"{m}_media"]:
            return "NA"
        return f"{a[f'{m}_media'] / c[f'{m}_media']:.2f}"

    for caso in sorted({p["caso"] for p in prom}):
        lineas.append(
            f"| {caso} | {razon(caso, 'pared_s')} | {razon(caso, 'cpu_s')} | "
            f"{razon(caso, 'espera_escritura_media_us')} | {razon(caso, 'espera_lectura_media_us')} |"
        )
    return "\n".join(lineas) + "\n"


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--rw", default="./rw", help="ejecutable (por defecto ./rw)")
    p.add_argument("--reps", type=int, default=5)
    p.add_argument("--salida", type=Path, default=Path("resultados/e2"))
    p.add_argument("--limite", type=float, default=300, help="segundos por ejecución")
    a = p.parse_args()

    rw = comando_rw(a.rw)
    entorno = registrar_entorno(a.salida / "entorno.txt", rw, {"repeticiones": a.reps})
    casos = list(casos_e2())
    total = a.reps * len(casos) * len(VARIANTES_E2)
    filas = []
    # Se alternan variantes dentro de cada repetición para que una variación
    # transitoria de la máquina no recaiga en una sola variante.
    for rep in range(1, a.reps + 1):
        for n, L, E, K, dl, de in casos:
            for modo, politica in VARIANTES_E2:
                argumentos = [modo, politica, *map(str, (L, E, K, dl, de)), "notraza"]
                codigo, out, err, _ = ejecutar(rw, argumentos, a.limite)
                nombre = f"crudo/c{n}_{modo}_{politica}_r{rep}.txt"
                guardar_crudo(a.salida / nombre, rw, argumentos, codigo, out, err)
                fila = {
                    **analizar_salida(out), "caso": n, "L": L, "E": E, "K": K,
                    "dur_lec_us": dl, "dur_esc_us": de, "modo": modo,
                    "politica": politica, "repeticion": rep, "codigo": codigo,
                    "archivo": nombre,
                }
                try:
                    fila["cpu_por_pared"] = float(fila["cpu_s"]) / float(fila["pared_s"])
                except (KeyError, ValueError, ZeroDivisionError):
                    fila["cpu_por_pared"] = ""
                filas.append(fila)
                estado = "OK" if valida(fila) else f"FALLA codigo={codigo}"
                print(f"[{len(filas):2}/{total}] caso {n} {modo:6} rep {rep}: {estado} "
                      f"pared={fila.get('pared_s', '?')} cpu={fila.get('cpu_s', '?')}",
                      flush=True)

    escribir_csv(a.salida / "e2.csv", filas, CAMPOS)
    prom = promediar(filas)
    escribir_csv(a.salida / "promedios.csv", prom, list(prom[0].keys()))
    (a.salida / "resumen.md").write_text(resumen(prom, entorno), encoding="utf-8")
    malas = sum(not valida(f) for f in filas)
    print(f"E2: {len(filas) - malas}/{len(filas)} válidas. Resumen en {a.salida / 'resumen.md'}")
    if malas:
        print("Hay ejecuciones con fallos: no usar estos resultados como definitivos.")
    raise SystemExit(1 if malas else 0)


if __name__ == "__main__":
    main()
