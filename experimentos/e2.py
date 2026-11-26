"""E2: costo de sincronización.

cond escritores frente a atomic escritores, 5 casos x 5 repeticiones = 50
ejecuciones con notraza. En el caso sin pausas (L=E=8) se usa K=10000.
Genera:
  <salida>/crudo/*.txt   salida original de cada ejecución
  <salida>/e2.csv        una fila por ejecución (datos originales, también las fallidas)
  <salida>/promedios.csv promedio y desviación de las repeticiones válidas
  <salida>/resumen.md    tablas para el informe
  <salida>/entorno.txt   máquina, compilador, commit y huellas
Una ejecución es válida si termina con código 0, pasa las comprobaciones y
trae todas las métricas con formato numérico. Los agregados usan solo las
válidas; un grupo sin válidas queda como NA. Termina con código 1 si alguna
ejecución no es válida: esos resultados no deben usarse como definitivos.
"""
import argparse
from pathlib import Path

from comun import (
    CASO_SIN_PAUSAS, CASOS, K_E2_SIN_PAUSAS, VARIANTES_E2, analizar_salida,
    comando_rw, desviacion, ejecutar, es_numero, escribir_csv, guardar_crudo,
    media, preparar_salida, registrar_entorno, repeticiones,
)

METRICAS = [
    "pared_s", "cpu_s", "espera_lectura_media_us", "espera_lectura_max_us",
    "espera_escritura_media_us", "espera_escritura_max_us", "max_lectoras",
    "fallos_recurso_ocupado", "fallos_escritoras_esperando", "fallos_cas",
]
CAMPOS = [
    "caso", "L", "E", "K", "dur_lec_us", "dur_esc_us", "modo", "politica",
    "repeticion", "codigo", "valida", "motivo", "r2_ok", "correcto", "traza",
    "solicitudes_lectura", "solicitudes_escritura", *METRICAS, "cpu_por_pared",
    "fallos_saturados", "archivo",
]


def casos_e2():
    for n, (L, E, K, dl, de) in enumerate(CASOS, 1):
        yield n, L, E, (K_E2_SIN_PAUSAS if n == CASO_SIN_PAUSAS else K), dl, de


def motivo_invalida(fila):
    """Devuelve por qué la ejecución no es válida, o cadena vacía si lo es."""
    motivos = []
    if fila["codigo"] != 0:
        motivos.append(f"codigo={fila['codigo']}")
    for campo, esperado in (("r2_ok", "SI"), ("correcto", "SI"),
                            ("traza", "desactivada"), ("fallos_saturados", "NO")):
        if fila.get(campo) != esperado:
            motivos.append(f"{campo}={fila.get(campo, 'falta')}")
    if fila.get("solicitudes_lectura") != str(fila["L"] * fila["K"]):
        motivos.append("solicitudes_lectura")
    if fila.get("solicitudes_escritura") != str(fila["E"] * fila["K"]):
        motivos.append("solicitudes_escritura")
    faltantes = [m for m in METRICAS if not es_numero(fila.get(m))]
    if faltantes:
        motivos.append("faltan " + ",".join(faltantes))
    elif float(fila["pared_s"]) <= 0:
        motivos.append("pared_s<=0")
    return ";".join(motivos)


def promediar(filas):
    grupos = {}
    for f in filas:
        grupos.setdefault((f["caso"], f["modo"]), []).append(f)
    salida = []
    for (caso, modo), grupo in sorted(grupos.items()):
        g0 = grupo[0]
        validas = [f for f in grupo if f["valida"] == "SI"]
        fila = {k: g0[k] for k in ("caso", "L", "E", "K", "dur_lec_us", "dur_esc_us", "modo", "politica")}
        fila["repeticiones"] = len(grupo)
        fila["validas"] = len(validas)
        for m in [*METRICAS, "cpu_por_pared"]:
            valores = [float(f[m]) for f in validas]
            fila[f"{m}_media"] = media(valores)
            fila[f"{m}_desv"] = desviacion(valores)
        salida.append(fila)
    return salida


def _fmt(v, dec=3):
    return "NA" if v is None else f"{v:.{dec}f}"


def _fmt_pm(p, m, dec):
    if p[f"{m}_media"] is None:
        return "NA"
    return f"{_fmt(p[f'{m}_media'], dec)} ± {_fmt(p[f'{m}_desv'], dec)}"


def resumen(prom, entorno, invalidas):
    por = {(p["caso"], p["modo"]): p for p in prom}
    lineas = [
        "# E2: costo de sincronización",
        "",
        f"Checkout `{entorno['commit_checkout']}` (rama `{entorno['rama']}`), "
        f"huella C++ `{entorno['huella_cpp']}`, huella scripts `{entorno['huella_scripts']}`, "
        f"{entorno['fecha']}, {entorno['cpu']} ({entorno['cpus_logicas']} CPU lógicas), "
        f"{entorno['compilador']}.",
        "",
        "Promedio ± desviación estándar de las repeticiones válidas, `notraza`. "
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
            f"{_fmt_pm(p, 'pared_s', 4)} | {_fmt_pm(p, 'cpu_s', 4)} | "
            f"{_fmt(p['cpu_por_pared_media'], 2)} |"
        )
    lineas += [
        "",
        "## Esperas por solicitud (µs, desde antes de `entrar_*()` hasta su retorno)",
        "",
        "Media: promedio de las medias por ejecución. Máx.: promedio de los máximos "
        "por ejecución (el mayor valor individual está en `e2.csv`).",
        "",
        "| Caso | Variante | Lectura media | Lectura: prom. de máximos | Escritura media | "
        "Escritura: prom. de máximos | Máx. lectoras (prom.) |",
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
                f"| {p['caso']} | {_fmt(p['fallos_recurso_ocupado_media'], 0)} | "
                f"{_fmt(p['fallos_escritoras_esperando_media'], 0)} | "
                f"{_fmt(p['fallos_cas_media'], 0)} |"
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
        if not c or not a or not c[f"{m}_media"] or a[f"{m}_media"] is None:
            return "NA"
        return f"{a[f'{m}_media'] / c[f'{m}_media']:.2f}"

    for caso in sorted({p["caso"] for p in prom}):
        lineas.append(
            f"| {caso} | {razon(caso, 'pared_s')} | {razon(caso, 'cpu_s')} | "
            f"{razon(caso, 'espera_escritura_media_us')} | {razon(caso, 'espera_lectura_media_us')} |"
        )
    lineas += ["", "## Ejecuciones no válidas", ""]
    if not invalidas:
        lineas.append("Ninguna.")
    for f in invalidas:
        lineas.append(f"- caso {f['caso']}, {f['modo']}, repetición {f['repeticion']}: "
                      f"{f['motivo']} (ver `{f['archivo']}`)")
    return "\n".join(lineas) + "\n"


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--rw", default="./rw", help="ejecutable (por defecto ./rw)")
    p.add_argument("--reps", type=repeticiones, default=5)
    p.add_argument("--salida", type=Path, required=True,
                   help="carpeta nueva o vacía para esta corrida")
    p.add_argument("--limite", type=float, default=300, help="segundos por ejecución")
    a = p.parse_args()

    rw = comando_rw(a.rw)
    preparar_salida(a.salida)
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
                fila["motivo"] = motivo_invalida(fila)
                fila["valida"] = "NO" if fila["motivo"] else "SI"
                fila["cpu_por_pared"] = (
                    float(fila["cpu_s"]) / float(fila["pared_s"]) if fila["valida"] == "SI" else ""
                )
                filas.append(fila)
                estado = "OK" if fila["valida"] == "SI" else f"NO VÁLIDA {fila['motivo']}"
                print(f"[{len(filas):2}/{total}] caso {n} {modo:6} rep {rep}: {estado} "
                      f"pared={fila.get('pared_s', '?')} cpu={fila.get('cpu_s', '?')}",
                      flush=True)

    escribir_csv(a.salida / "e2.csv", filas, CAMPOS)
    prom = promediar(filas)
    escribir_csv(a.salida / "promedios.csv", prom, list(prom[0].keys()))
    invalidas = [f for f in filas if f["valida"] != "SI"]
    (a.salida / "resumen.md").write_text(resumen(prom, entorno, invalidas), encoding="utf-8")
    print(f"E2: {total - len(invalidas)}/{total} válidas. Resumen en {a.salida / 'resumen.md'}")
    if invalidas:
        print("Hay ejecuciones no válidas: no usar estos resultados como definitivos.")
    raise SystemExit(1 if invalidas else 0)


if __name__ == "__main__":
    main()
