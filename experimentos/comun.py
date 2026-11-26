"""Definiciones compartidas por los experimentos E1 y E2 (P4).

Los casos y variantes son los del enunciado. Cada ejecución guarda su salida
original en un archivo propio para que los resultados sean rastreables.
"""
import argparse
import csv
import datetime
import hashlib
import math
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
SIN_ATRIBUIR = "sin atribuir"


def repeticiones(texto):
    valor = int(texto)
    if valor < 1:
        raise argparse.ArgumentTypeError("--reps debe ser al menos 1")
    return valor


def preparar_salida(ruta):
    """Rechaza una carpeta con resultados previos para no mezclar ni pisar corridas."""
    if ruta.exists() and any(ruta.iterdir()):
        raise SystemExit(
            f"La carpeta {ruta} ya contiene resultados. Use otra carpeta "
            f"(RESULTADOS=...) o bórrela si de verdad quiere reemplazarlos."
        )
    ruta.mkdir(parents=True, exist_ok=True)


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


def es_entero(valor):
    return isinstance(valor, str) and valor.isdigit()


def es_numero(valor):
    try:
        return math.isfinite(float(valor))
    except (TypeError, ValueError):
        return False


def _texto(valor):
    if valor is None:
        return ""
    return valor.decode(errors="replace") if isinstance(valor, bytes) else valor


def ejecutar(rw, argumentos, limite_s):
    inicio = time.monotonic()
    try:
        proceso = subprocess.run(
            [*rw, *argumentos], capture_output=True, text=True, timeout=limite_s
        )
        codigo, stdout, stderr = proceso.returncode, proceso.stdout, proceso.stderr
    except subprocess.TimeoutExpired as e:
        codigo = "TIMEOUT"
        stdout = _texto(e.stdout)
        # Se conserva lo que el proceso alcanzó a escribir antes del límite.
        stderr = _texto(e.stderr) + f"Superado el límite de {limite_s} s\n"
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
    return statistics.fmean(valores) if valores else None


def desviacion(valores):
    if not valores:
        return None
    return statistics.stdev(valores) if len(valores) > 1 else 0.0


def _directorio_git():
    """Ubica el directorio de git, también en un worktree donde .git es un archivo."""
    git = RAIZ / ".git"
    if git.is_file():
        texto = git.read_text().strip()
        if texto.startswith("gitdir: "):
            git = (RAIZ / texto[8:]).resolve()
    comun = git / "commondir"
    return git, ((git / comun.read_text().strip()).resolve() if comun.exists() else git)


def _commit_actual():
    """Lee el commit del checkout desde los archivos de git."""
    try:
        git, comun = _directorio_git()
        cabeza = (git / "HEAD").read_text().strip()
        if not cabeza.startswith("ref: "):
            return cabeza, "(HEAD separado)"
        ref = cabeza[5:]
        rama = ref.rsplit("refs/heads/", 1)[-1]
        for base in (git, comun):
            if (base / ref).exists():
                return (base / ref).read_text().strip(), rama
        empaquetadas = comun / "packed-refs"
        if empaquetadas.exists():
            for linea in empaquetadas.read_text().splitlines():
                if linea.endswith(" " + ref):
                    return linea.split()[0], rama
        return "desconocido", rama
    except OSError:
        return "desconocido", "desconocida"


def huella(*patrones):
    """SHA-256 de los archivos indicados, con fines de línea normalizados.

    Identifica el contenido medido aunque haya cambios sin commit o el commit
    con los scripts sea posterior al del C++ medido.
    """
    h = hashlib.sha256()
    for ruta in sorted({p for patron in patrones for p in RAIZ.glob(patron) if p.is_file()}):
        h.update(ruta.relative_to(RAIZ).as_posix().encode() + b"\0")
        h.update(ruta.read_bytes().replace(b"\r\n", b"\n") + b"\0")
    return h.hexdigest()[:16]


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
        "commit_checkout": commit,
        "rama": rama,
        "huella_cpp": huella("src/*.cpp", "include/*.hpp"),
        "huella_scripts": huella("experimentos/*.py"),
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
