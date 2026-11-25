"""Definiciones compartidas por los experimentos E1 y E2 (P4).

Los casos y variantes son los del enunciado. Cada ejecución guarda su salida
original en un archivo propio para que los resultados sean rastreables.
"""
import csv
import datetime
import os
import platform
import shlex
import statistics
import subprocess
import sys
import time
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent

# (L, E, K, dur_lec_us, dur_esc_us), numerados desde 1 como en el enunciado.
CASOS = [
    (1, 1, 20, 500, 500),
    (6, 2, 100, 200, 500),
    (2, 6, 100, 500, 200),
    (8, 8, 100, 0, 0),
    (8, 1, 200, 1000, 100),
]
CASO_SIN_PAUSAS = 4
K_E2_SIN_PAUSAS = 10000

VARIANTES_E1 = [
    ("cond", "lectores"),
    ("cond", "escritores"),
    ("cond", "fifo"),
    ("atomic", "escritores"),
]
VARIANTES_E2 = [
    ("cond", "escritores"),
    ("atomic", "escritores"),
]

# Responsable de cada módulo según la distribución, para remitir errores.
RESPONSABLE_R2 = {
    ("cond", "lectores"): "P1",
    ("cond", "escritores"): "P1",
    ("cond", "fifo"): "P2",
    ("atomic", "escritores"): "P3",
}
RESPONSABLE_POLITICA = "P2"


def comando_rw(texto):
    """Permite pasar un ejecutable con argumentos previos (p. ej. un envoltorio)."""
    partes = shlex.split(texto)
    if not partes:
        raise SystemExit("--rw no puede ser vacío")
    if len(partes) == 1:
        partes[0] = str(Path(partes[0]).resolve())
    return partes


def analizar_salida(stdout):
    return dict(par.split("=", 1) for par in stdout.split() if "=" in par)


def ejecutar(rw, argumentos, limite_s):
    inicio = time.monotonic()
    try:
        proceso = subprocess.run(
            [*rw, *argumentos], capture_output=True, text=True, timeout=limite_s
        )
        codigo, stdout, stderr = proceso.returncode, proceso.stdout, proceso.stderr
    except subprocess.TimeoutExpired as e:
        codigo = "TIMEOUT"
        stdout = e.stdout.decode() if isinstance(e.stdout, bytes) else (e.stdout or "")
        stderr = f"Superado el límite de {limite_s} s"
    return codigo, stdout, stderr, time.monotonic() - inicio


def guardar_crudo(ruta, rw, argumentos, codigo, stdout, stderr):
    ruta.parent.mkdir(parents=True, exist_ok=True)
    ruta.write_text(
        f"$ {shlex.join([*rw, *argumentos])}\n"
        f"# codigo_salida={codigo}\n"
        f"# --- stdout\n{stdout}"
        f"# --- stderr\n{stderr}",
        encoding="utf-8",
    )


def escribir_csv(ruta, filas, campos):
    ruta.parent.mkdir(parents=True, exist_ok=True)
    with ruta.open("w", newline="", encoding="utf-8") as f:
        escritor = csv.DictWriter(f, fieldnames=campos, extrasaction="ignore")
        escritor.writeheader()
        escritor.writerows(filas)


def leer_csv(ruta):
    with Path(ruta).open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def media(valores):
    return statistics.fmean(valores) if valores else float("nan")


def desviacion(valores):
    return statistics.stdev(valores) if len(valores) > 1 else 0.0


def _commit_actual():
    """Lee el commit desde .git sin invocar git."""
    git = RAIZ / ".git"
    try:
        cabeza = (git / "HEAD").read_text().strip()
        if not cabeza.startswith("ref: "):
            return cabeza, "(HEAD separado)"
        ref = cabeza[5:]
        rama = ref.rsplit("refs/heads/", 1)[-1]
        archivo = git / ref
        if archivo.exists():
            return archivo.read_text().strip(), rama
        empaquetadas = git / "packed-refs"
        if empaquetadas.exists():
            for linea in empaquetadas.read_text().splitlines():
                if linea.endswith(" " + ref):
                    return linea.split()[0], rama
        return "desconocido", rama
    except OSError:
        return "desconocido", "desconocida"


def _salida(comando):
    try:
        return subprocess.run(
            comando, capture_output=True, text=True, timeout=10
        ).stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return "no disponible"


def _modelo_cpu():
    try:
        for linea in Path("/proc/cpuinfo").read_text().splitlines():
            if linea.startswith("model name"):
                return linea.split(":", 1)[1].strip()
    except OSError:
        pass
    return platform.processor() or "no disponible"


def registrar_entorno(ruta, rw, extra=None):
    commit, rama = _commit_actual()
    gxx = _salida([os.environ.get("CXX", "g++"), "--version"]).splitlines()
    lineas = {
        "fecha": datetime.datetime.now().isoformat(timespec="seconds"),
        "commit": commit,
        "rama": rama,
        "sistema": " ".join(platform.uname()),
        "wsl": "SI" if "microsoft" in platform.release().lower() else "NO",
        "cpu": _modelo_cpu(),
        "cpus_logicas": os.cpu_count(),
        "compilador": gxx[0] if gxx else "no disponible",
        "python": sys.version.split()[0],
        "ejecutable": shlex.join(rw),
        **(extra or {}),
    }
    ruta.parent.mkdir(parents=True, exist_ok=True)
    ruta.write_text(
        "".join(f"{k}: {v}\n" for k, v in lineas.items()), encoding="utf-8"
    )
    return lineas
