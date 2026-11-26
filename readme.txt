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

Cada experimento guarda en <carpeta>/<e1|e2>/entorno.txt la fecha, el
commit, las huellas del codigo, la CPU, el numero de CPU logicas y la
version del compilador.


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

Carpetas de resultados:

  resultados/                      medicion publicada, la que cita el informe
  material-local/experimentos/<fecha-hora>/
                                   cada nueva corrida (ignorada por git)

make experimentos escribe por defecto en una carpeta nueva bajo
material-local/, asi que reproducir no pisa la medicion publicada. Los
scripts se niegan a escribir en una carpeta que ya tiene resultados.
Variables opcionales: REPS=5, RESULTADOS=<carpeta>, PYTHON=python3.

  make experimentos REPS=1          prueba rapida (REPS debe ser >= 1)
  make graficos RESULTADOS=<carpeta de una corrida>

make experimentos ejecuta E1, E2 y graficos en orden, tambien con make -j, y
se detiene antes de E2 si alguna ejecucion de E1 falla, para no medir una
version con fallos.

Archivos de cada corrida (<carpeta> = resultados/ o material-local/...):

  e1/crudo/          salida original de cada ejecucion (comando, codigo de
                     salida, stdout y stderr, incluso si hubo timeout)
  e1/e1.csv          una fila por ejecucion y sus comprobaciones
  e1/resumen.md      tabla por caso/variante, evidencia y errores con el
                     responsable del modulo (P1, P2, P3 o "sin atribuir")
  e2/crudo/          salida original de cada ejecucion
  e2/e2.csv          datos originales de las 50 mediciones, tambien las no validas
  e2/promedios.csv   promedio y desviacion de las repeticiones validas
                     (NA si un grupo no tiene ninguna)
  e2/resumen.md      tablas de tiempos, esperas, fallos atomicos, razon
                     atomic/cond y ejecuciones no validas
  e2/graficos/       un PNG por metrica de tiempo y espera (pared, CPU,
                     espera media y maxima de lectoras y de escritoras)
  */entorno.txt      fecha, commit del checkout, huellas, CPU y compilador

Metodo de medicion (E2): tiempo de pared con std::chrono::steady_clock y CPU
del proceso con CLOCK_PROCESS_CPUTIME_ID, ambos desde antes de crear las
hebras hasta completar todos los join(). La espera de cada solicitud se mide
desde antes de llamar a entrar_*() hasta su retorno. Las repeticiones se
alternan entre variantes para repartir perturbaciones de la maquina.

Las esperas maximas de resumen.md y de los graficos son el promedio de los
maximos de cada ejecucion; el mayor valor individual esta en e2.csv.

Reproducir la medicion publicada:

  entorno.txt registra dos huellas SHA-256 (16 primeros caracteres):
  huella_cpp sobre src/*.cpp e include/*.hpp (el programa medido) y
  huella_scripts sobre experimentos/*.py. Basta usar cualquier commit que
  contenga experimentos/ y cuya huella_cpp coincida con la de
  resultados/e2/entorno.txt: ese commit tiene el mismo C++ medido y los
  scripts para repetir la tanda. Al correr make experimentos, compare la
  huella_cpp de la nueva corrida con la publicada. commit_checkout indica
  el commit que estaba en el checkout al medir; no refleja cambios sin
  commit, por eso se usan las huellas.

Los tiempos dependen de la maquina y su carga; conviene ejecutar con el
equipo en reposo y conectado a la corriente.
