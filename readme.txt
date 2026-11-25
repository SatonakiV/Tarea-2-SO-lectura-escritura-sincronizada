Tarea 2: Lectores y escritores - Sistemas Operativos 2026
=========================================================

Integrantes y reparto
---------------------
P1  Diego Fuentes            Base comun, registro, monitor (prioridad lectores/escritores)
P2  Ignacio Placencia Palma  FIFO, trazas de admision y verificador
P3  Isaac Castro             ControlAtomicoRW (CAS) y contadores de fallos
P4  Joaquin Adauy            Experimentos E1/E2, reproducibilidad e informe

Repositorio: https://github.com/SatonakiV/Tarea-2-SO-lectura-escritura-sincronizada


1. Entorno
----------
Linux (o WSL2 con Ubuntu). La medicion de CPU usa clock_gettime con
CLOCK_PROCESS_CPUTIME_ID, por lo que se requiere un sistema POSIX.

  - g++ con C++17 y GNU Make
  - Python 3 y GNU Coreutils (timeout) para pruebas y experimentos
  - matplotlib, solo para los graficos (opcional)

En Ubuntu/WSL:

  sudo apt install build-essential python3 python3-matplotlib

Cada experimento guarda en resultados/<e1|e2>/entorno.txt la fecha, el
commit, la CPU, el numero de CPU logicas y la version del compilador.


2. Compilar y ejecutar
----------------------
  make
  ./rw <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]

  modo      cond | atomic
  politica  lectores | escritores | fifo   (atomic solo admite escritores)
  L, E      lectoras y escritoras (>= 1, L + E <= 1000)
  K         operaciones por hebra (>= 1)
  dur_*_us  duracion simulada de cada lectura/escritura, en microsegundos
  notraza   desactiva la traza de admision (se usa en E2)

Ejemplos:
  ./rw cond fifo 6 2 100 200 500
  ./rw atomic escritores 8 8 10000 0 0 notraza

La salida es una lista de pares clave=valor. r2_ok resume registro, carga y
exclusion; politica_ok el resultado del verificador de trazas; correcto=SI
indica que no se detecto ningun fallo. Codigo de salida 0 = sin fallos.
El detalle de cada campo esta en README.md.


3. Pruebas
----------
  make test         pruebas unitarias, FIFO, verificador y CLI
  make test-tsan    pruebas con ThreadSanitizer


4. Experimentos (reproducen los resultados del informe)
--------------------------------------------------------
  make experimentos     E1, luego E2, luego graficos

o por separado:

  make e1               100 ejecuciones: 5 casos x 4 variantes x 5 repeticiones
  make e2               50 ejecuciones: cond escritores y atomic escritores,
                        5 casos x 5 repeticiones, notraza, K=10000 en el caso 4
  make graficos         PNG de E2 a partir de los promedios

Casos (enunciado):

  Caso  L  E  K    lectura us  escritura us
  1     1  1  20   500         500
  2     6  2  100  200         500
  3     2  6  100  500         200
  4     8  8  100  0           0      (E2: K=10000)
  5     8  1  200  1000        100

Variables opcionales: REPS=5, RESULTADOS=resultados, PYTHON=python3.
Por ejemplo, una prueba rapida que no pisa los datos del informe:

  make experimentos REPS=1 RESULTADOS=/tmp/prueba

make e1 termina con error si alguna ejecucion falla, y make experimentos se
detiene antes de E2 en ese caso, para no medir una version con fallos.

Archivos generados:

  resultados/e1/crudo/       salida original de cada ejecucion (comando,
                             codigo de salida, stdout y stderr)
  resultados/e1/e1.csv       una fila por ejecucion y comprobaciones
  resultados/e1/resumen.md   tabla por caso/variante, evidencia y errores
                             con el responsable del modulo (P1, P2 o P3)
  resultados/e2/crudo/       salida original de cada ejecucion
  resultados/e2/e2.csv       datos originales de las 50 mediciones
  resultados/e2/promedios.csv promedio y desviacion de las 5 repeticiones
  resultados/e2/resumen.md   tablas de tiempos, esperas, fallos atomicos y
                             razon atomic/cond
  resultados/e2/graficos/    un PNG por metrica

Metodo de medicion (E2): tiempo de pared con std::chrono::steady_clock y CPU
del proceso con CLOCK_PROCESS_CPUTIME_ID, ambos desde antes de crear las
hebras hasta completar todos los join(). La espera de cada solicitud se mide
desde antes de llamar a entrar_*() hasta su retorno. Las repeticiones se
alternan entre variantes para repartir perturbaciones de la maquina.

Para reproducir sobre la misma version, compilar el commit indicado en
resultados/e2/entorno.txt. Los tiempos dependen de la maquina y su carga;
conviene ejecutar con el equipo en reposo y conectado a la corriente.
