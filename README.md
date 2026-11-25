# Tarea 2: lectores y escritores

## Requisitos

- Linux o un entorno POSIX compatible.
- GCC con soporte para C++17 y GNU Make.
- Python 3 y GNU Coreutils (`timeout`) para ejecutar las pruebas.

## Compilar

Desde la carpeta del proyecto:

```sh
make
```

Se genera el ejecutable `rw`.

## Ejecutar

```sh
./rw <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]
```

| Argumento | Descripción |
| --- | --- |
| `modo` | `cond` para monitor con variables de condición; `atomic` para operaciones atómicas. |
| `politica` | `lectores`, `escritores` o `fifo`. El modo `atomic` solo admite `escritores`. |
| `L` | Cantidad de lectoras, al menos 1. |
| `E` | Cantidad de escritoras, al menos 1. La suma `L + E` debe ser como máximo 1000. |
| `K` | Operaciones por hebra, al menos 1. |
| `dur_lec_us` | Duración simulada de cada lectura, en microsegundos, mayor o igual a 0. |
| `dur_esc_us` | Duración simulada de cada escritura, en microsegundos, mayor o igual a 0. |
| `notraza` | Opción para solicitar una ejecución sin traza de admisión. |

Los valores numéricos deben ser enteros decimales sin signo y caber en `long`.
También deben caber en `long` los productos `2 * E * K` y `(L + E) * K`.
Una duración de cero ejecuta la operación sin pausa simulada.

Ejemplos:

```sh
./rw cond lectores 6 2 100 200 500
./rw cond escritores 6 2 100 200 500
./rw cond escritores 8 8 10000 0 0 notraza
./rw atomic escritores 6 2 100 200 500
```

En modo `atomic`, las hebras que no pueden entrar esperan activamente y el
resumen informa los intentos fallidos por causa: `fallos_recurso_ocupado`,
`fallos_escritoras_esperando` y `fallos_cas`.

El programa imprime un resumen con las operaciones, valores finales del registro,
comprobaciones y métricas de tiempo y concurrencia. Los errores se escriben en la
salida de error. `r2_ok` indica si pasan las comprobaciones del registro, la carga
y la exclusión. Si pasan pero la verificación de política está pendiente, se muestra
`correcto=PENDIENTE`; cualquier fallo detectado produce `correcto=NO`. Una política
verificada sin errores permite `correcto=SI`. Con `notraza` (o en modo `atomic`),
`politica_ok=NA` y `correcto` refleja solo las comprobaciones de ejecución.
Los errores de política incluyen en la salida de error el índice de la anotación
(desde cero) y la regla incumplida.
El código de salida es 0 si la ejecución termina sin fallos detectados, incluso
si la verificación de política está pendiente, y
1 si hay argumentos inválidos, una selección no disponible o un fallo de ejecución.

## Probar

```sh
make test
```

Cada proceso de pruebas tiene un límite de 60 segundos. Para ampliarlo:

```sh
make test TEST_TIMEOUT=120s
```

Al vencer el límite se solicita la terminación; si el proceso no termina en otros
5 segundos, se fuerza su cierre. La ejecución de pruebas devuelve un error.

## Experimentos

```sh
make experimentos   # E1 (100 ejecuciones), E2 (50 ejecuciones) y gráficos
make e1             # solo E1
make e2             # solo E2
```

Los resultados quedan en `resultados/`: salida original de cada ejecución,
CSV por ejecución, promedios, resúmenes en Markdown y gráficos de E2 (requiere
`matplotlib`). `make experimentos` se detiene antes de E2 si E1 detecta un fallo.
`REPS` y `RESULTADOS` permiten una prueba rápida sin pisar los datos, por ejemplo
`make experimentos REPS=1 RESULTADOS=/tmp/prueba`. El detalle está en `readme.txt`.
