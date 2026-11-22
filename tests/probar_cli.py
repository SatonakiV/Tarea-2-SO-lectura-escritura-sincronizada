"""Regresión de la interfaz pública; no sustituye los experimentos E1/E2 de P4."""
import argparse
import json
from pathlib import Path
import subprocess


def exigir(condicion, detalle):
    if not condicion:
        raise RuntimeError(str(detalle))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ejecutable")
    parser.add_argument("--registro", type=Path)
    args = parser.parse_args()
    ejecutable = str(Path(args.ejecutable).resolve())
    registros = []

    def ejecutar(argumentos, codigo):
        proceso = subprocess.run(
            [ejecutable, *argumentos],
            capture_output=True,
            text=True,
            timeout=30
        )
        registros.append(dict(
            argumentos=argumentos,
            codigo=proceso.returncode,
            stdout=proceso.stdout,
            stderr=proceso.stderr
        ))
        exigir(proceso.returncode == codigo, registros[-1])

        return dict(
            par.split("=", 1)
            for par in proceso.stdout.split()
            if "=" in par
        )

    try:
        for politica in ("lectores", "escritores", "fifo"):
            for opcion in ([], ["notraza"]):
                campos = ejecutar(
                    ["cond", politica, "6", "2", "40", "100", "150", *opcion],
                    0
                )

                exigir(campos["version_final"] == "80", campos)
                exigir(campos["doble_version_final"] == "160", campos)
                exigir(campos["operaciones_realizadas"] == "320", campos)
                exigir(
                    campos["inconsistencias"] ==
                    campos["violaciones_exclusion"] == "0",
                    campos
                )
                exigir(campos["r2_ok"] == "SI", campos)
                exigir(campos["correcto"] == "SI", campos)
                exigir(campos["solicitudes_lectura"] == "240", campos)
                exigir(campos["solicitudes_escritura"] == "80", campos)
                exigir(
                    campos["politica_ok"] == ("NA" if opcion else "SI"),
                    campos
                )
                exigir(
                    campos["traza"] == (
                        "desactivada" if opcion else "activa"
                    ),
                    campos
                )

                if opcion:
                    exigir(campos["evidencia"] == "NA", campos)
                else:
                    exigir(int(campos["evidencia"]) >= 0, campos)

                exigir(1 <= int(campos["max_lectoras"]) <= 6, campos)
                exigir(
                    float(campos["pared_s"]) > 0 and
                    float(campos["cpu_s"]) >= 0,
                    campos
                )

        for caso, version in (
            (["6", "2", "40", "100", "150"], 80),
            (["1", "1", "1", "0", "0"], 1)
        ):
            for opcion in ([], ["notraza"]):
                campos = ejecutar(
                    ["atomic", "escritores", *caso, *opcion],
                    0
                )

                exigir(campos["version_final"] == str(version), campos)
                exigir(
                    campos["doble_version_final"] == str(2 * version),
                    campos
                )
                exigir(
                    campos["inconsistencias"] ==
                    campos["violaciones_exclusion"] == "0",
                    campos
                )
                exigir(
                    campos["r2_ok"] == campos["correcto"] == "SI",
                    campos
                )
                exigir(campos["fallos_aplican"] == "SI", campos)
                exigir(campos["fallos_saturados"] == "NO", campos)
                exigir(
                    campos["politica_ok"] == "NA" and
                    campos["traza"] == "desactivada",
                    campos
                )

                for causa in (
                    "fallos_recurso_ocupado",
                    "fallos_escritoras_esperando",
                    "fallos_cas"
                ):
                    exigir(int(campos[causa]) >= 0, campos)

        ejecuciones = len(registros)
        base = ["cond", "lectores", "1", "1", "1", "0", "0"]
        invalidos = [
            [],
            base + ["extra"],
            base + ["notraza", "extra"]
        ]

        for indice, valor in (
            (0, "otro"),
            (1, "otra"),
            (2, "0"),
            (3, "0"),
            (4, "0"),
            (5, "-1"),
            (6, "-1"),
            (2, "1x"),
            (2, "99999999999999999999999999999"),
            (4, "9223372036854775807")
        ):
            caso = base.copy()
            caso[indice] = valor
            invalidos.append(caso)

        invalidos += [
            ["cond", "lectores", "999", "2", "1", "0", "0"],
            ["atomic", "lectores", "1", "1", "1", "0", "0"],
            ["atomic", "fifo", "1", "1", "1", "0", "0"]
        ]

        for caso in invalidos:
            ejecutar(caso, 1)

        print(
            f"CLI: {len(registros)} casos OK "
            f"({ejecuciones} ejecuciones y {len(invalidos)} rechazos)"
        )
    finally:
        if args.registro:
            args.registro.parent.mkdir(
                parents=True,
                exist_ok=True
            )
            args.registro.write_text(
                json.dumps(registros, ensure_ascii=False, indent=2),
                encoding="utf-8"
            )


if __name__ == "__main__":
    main()
