"""E1: corrección y prioridades.

5 casos x 4 variantes x 5 repeticiones = 100 ejecuciones con traza activa.
Genera:
  <salida>/crudo/*.txt   salida original de cada ejecución
  <salida>/e1.csv        una fila por ejecución
  <salida>/resumen.md    tabla consolidada y errores por responsable
  <salida>/entorno.txt   máquina, compilador y commit
Termina con código 1 si alguna ejecución no cumple las comprobaciones.
"""
import argparse
from pathlib import Path

from comun import (
    CASOS, RESPONSABLE_POLITICA, RESPONSABLE_R2, VARIANTES_E1, comando_rw,
    analizar_salida, ejecutar, escribir_csv, guardar_crudo, media,
    registrar_entorno,
)

CAMPOS = [
    "caso", "L", "E", "K", "dur_lec_us", "dur_esc_us", "modo", "politica",
    "repeticion", "codigo", "ok", "fallas", "responsable", "r2_ok", "correcto",
    "politica_ok", "evidencia", "operaciones_realizadas", "version_final",
    "doble_version_final", "inconsistencias", "violaciones_exclusion",
    "diagnostico_vacio", "max_lectoras", "fallos_recurso_ocupado",
    "fallos_escritoras_esperando", "fallos_cas", "fallos_saturados", "pared_s",
    "error", "archivo",
]


def comprobar(fila, modo, L, E, K):
    """Devuelve las comprobaciones incumplidas y a quién remitirlas."""
    fallas, responsables = [], set()
    variante = (modo, fila["politica"])

    def exigir(condicion, detalle, responsable):
        if not condicion:
            fallas.append(detalle)
            responsables.add(responsable)

    r2 = RESPONSABLE_R2[variante]
    exigir(fila["codigo"] == 0, f"codigo={fila['codigo']}", r2)
    exigir(fila.get("r2_ok") == "SI", "r2_ok", r2)
    exigir(fila.get("version_final") == str(E * K), "version_final", r2)
    exigir(fila.get("doble_version_final") == str(2 * E * K), "doble_version_final", r2)
    exigir(fila.get("operaciones_realizadas") == str((L + E) * K), "operaciones", r2)
    exigir(fila.get("inconsistencias") == "0", "inconsistencias", r2)
    exigir(fila.get("violaciones_exclusion") == "0", "exclusion", r2)
    exigir(fila.get("diagnostico_vacio") == "SI", "diagnostico_vacio", r2)
    try:
        exigir(1 <= int(fila.get("max_lectoras", "-1")) <= L, "max_lectoras", r2)
    except ValueError:
        exigir(False, "max_lectoras", r2)
    if modo == "cond":
        exigir(fila.get("politica_ok") == "SI", "politica_ok", RESPONSABLE_POLITICA)
        exigir(fila.get("correcto") == "SI", "correcto", RESPONSABLE_POLITICA)
    else:
        exigir(fila.get("politica_ok") == "NA", "politica_ok", r2)
        exigir(fila.get("correcto") == "SI", "correcto", r2)
        exigir(fila.get("fallos_saturados") == "NO", "fallos_saturados", r2)
    return fallas, sorted(responsables)


def resumen(filas, entorno):
    total = len(filas)
    correctas = sum(f["ok"] == "SI" for f in filas)
    lineas = [
        "# E1: corrección y prioridades",
        "",
        f"Commit `{entorno['commit']}` (rama `{entorno['rama']}`), "
        f"{entorno['fecha']}, {entorno['cpu']} ({entorno['cpus_logicas']} CPU lógicas).",
        "",
        f"**Ejecuciones correctas: {correctas}/{total}.**",
        "",
        "Evidencia: indicador del verificador (min-máx en las repeticiones). "
        "Una evidencia igual a cero no demuestra un error: la regla pudo no "
        "ponerse a prueba en esa ejecución. El modo atómico no tiene traza; se "
        "informan sus fallos de admisión por causa (promedio).",
        "",
        "| Caso | L | E | K | lec µs | esc µs | Variante | OK | Evidencia | "
        "Máx. lectoras | Fallos ocupado | Fallos esc. esperando | Fallos CAS |",
        "|---|---|---|---|---|---|---|---|---|---|---|---|---|",
    ]
    grupos = {}
    for f in filas:
        grupos.setdefault((int(f["caso"]), f["modo"], f["politica"]), []).append(f)
    orden = {v: i for i, v in enumerate(VARIANTES_E1)}
    for (caso, modo, politica), grupo in sorted(
            grupos.items(), key=lambda g: (g[0][0], orden[(g[0][1], g[0][2])])):
        g0 = grupo[0]
        ok = sum(f["ok"] == "SI" for f in grupo)
        evid = [int(f["evidencia"]) for f in grupo if f["evidencia"].isdigit()]
        maxl = [int(f["max_lectoras"]) for f in grupo if f["max_lectoras"].isdigit()]
        if modo == "atomic":
            fallos = [
                f"{media([int(f[c]) for f in grupo if f[c].isdigit()]):.0f}"
                for c in ("fallos_recurso_ocupado", "fallos_escritoras_esperando", "fallos_cas")
            ]
        else:
            fallos = ["NA"] * 3
        lineas.append(
            f"| {caso} | {g0['L']} | {g0['E']} | {g0['K']} | {g0['dur_lec_us']} | "
            f"{g0['dur_esc_us']} | {modo} {politica} | {ok}/{len(grupo)} | "
            f"{f'{min(evid)}-{max(evid)}' if evid else 'NA'} | "
            f"{max(maxl) if maxl else 'NA'} | {' | '.join(fallos)} |"
        )
    erroneas = [f for f in filas if f["ok"] != "SI"]
    lineas += ["", "## Errores para remitir", ""]
    if not erroneas:
        lineas.append("Ninguno.")
    for f in erroneas:
        lineas.append(
            f"- {f['responsable']}: caso {f['caso']}, {f['modo']} {f['politica']}, "
            f"repetición {f['repeticion']}: {f['fallas']}"
            f"{' - ' + f['error'] if f['error'] else ''} (ver `{f['archivo']}`)"
        )
    return "\n".join(lineas) + "\n"


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--rw", default="./rw", help="ejecutable (por defecto ./rw)")
    p.add_argument("--reps", type=int, default=5)
    p.add_argument("--salida", type=Path, default=Path("resultados/e1"))
    p.add_argument("--limite", type=float, default=120, help="segundos por ejecución")
    a = p.parse_args()

    rw = comando_rw(a.rw)
    entorno = registrar_entorno(a.salida / "entorno.txt", rw, {"repeticiones": a.reps})
    filas = []
    total = a.reps * len(CASOS) * len(VARIANTES_E1)
    # Repeticiones en el exterior: una perturbación pasajera no afecta a una sola variante.
    for rep in range(1, a.reps + 1):
        for n, (L, E, K, dl, de) in enumerate(CASOS, 1):
            for modo, politica in VARIANTES_E1:
                argumentos = [modo, politica, *map(str, (L, E, K, dl, de))]
                codigo, out, err, _ = ejecutar(rw, argumentos, a.limite)
                nombre = f"crudo/c{n}_{modo}_{politica}_r{rep}.txt"
                guardar_crudo(a.salida / nombre, rw, argumentos, codigo, out, err)
                fila = {
                    **analizar_salida(out), "caso": n, "L": L, "E": E, "K": K,
                    "dur_lec_us": dl, "dur_esc_us": de, "modo": modo,
                    "politica": politica, "repeticion": rep, "codigo": codigo,
                    "archivo": nombre,
                    "error": " / ".join(l.strip() for l in err.splitlines() if l.strip()),
                }
                fallas, responsables = comprobar(fila, modo, L, E, K)
                fila.update(ok="NO" if fallas else "SI", fallas=";".join(fallas),
                            responsable=",".join(responsables))
                filas.append(fila)
                print(f"[{len(filas):3}/{total}] caso {n} {modo:6} {politica:10} "
                      f"rep {rep}: {'OK' if not fallas else 'FALLA ' + fila['fallas']}",
                      flush=True)

    escribir_csv(a.salida / "e1.csv", filas, CAMPOS)
    texto = resumen([{k: str(v) for k, v in f.items()} for f in filas], entorno)
    (a.salida / "resumen.md").write_text(texto, encoding="utf-8")
    malas = sum(f["ok"] != "SI" for f in filas)
    print(f"E1: {len(filas) - malas}/{len(filas)} correctas. Resumen en {a.salida / 'resumen.md'}")
    raise SystemExit(1 if malas else 0)


if __name__ == "__main__":
    main()
