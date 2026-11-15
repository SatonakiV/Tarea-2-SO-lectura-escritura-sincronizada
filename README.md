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
```

El programa imprime un resumen con las operaciones, valores finales del registro,
comprobaciones y métricas de tiempo y concurrencia. Los errores se escriben en la
salida de error. El código de salida es 0 si la ejecución termina correctamente y
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
