# E2: costo de sincronización

Checkout `9d314d9364d665f027a5ff286d7c47e5dd5c8fc4` (rama `rama.joaco`), huella C++ `d8a48c0f37d3d1cf`, huella scripts `d96402c5f1f9524b`, 2026-11-26T08:46:22, Intel(R) Core(TM) i5-10210U CPU @ 1.60GHz (8 CPU lógicas), g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0.

Promedio ± desviación estándar de las repeticiones válidas, `notraza`. Pared: `steady_clock`; CPU: `CLOCK_PROCESS_CPUTIME_ID`. Ambas cubren desde antes de crear las hebras hasta completar todos los `join()`. El tiempo de CPU incluye trabajo útil, no solo espera.

## Tiempos

| Caso | L | E | K | lec µs | esc µs | Variante | Válidas | Pared (s) | CPU (s) | CPU/pared |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 1 | 1 | 20 | 500 | 500 | atomic | 5/5 | 0.0300 ± 0.0003 | 0.0314 ± 0.0005 | 1.05 |
| 1 | 1 | 1 | 20 | 500 | 500 | cond | 5/5 | 0.0353 ± 0.0015 | 0.0063 ± 0.0004 | 0.18 |
| 2 | 6 | 2 | 100 | 200 | 500 | atomic | 5/5 | 0.1734 ± 0.0133 | 0.9955 ± 0.0929 | 5.74 |
| 2 | 6 | 2 | 100 | 200 | 500 | cond | 5/5 | 0.3209 ± 0.0095 | 0.1918 ± 0.0125 | 0.60 |
| 3 | 2 | 6 | 100 | 500 | 200 | atomic | 5/5 | 0.3260 ± 0.0664 | 1.7764 ± 0.4577 | 5.41 |
| 3 | 2 | 6 | 100 | 500 | 200 | cond | 5/5 | 0.3892 ± 0.0511 | 0.1212 ± 0.0336 | 0.31 |
| 4 | 8 | 8 | 10000 | 0 | 0 | atomic | 5/5 | 0.0426 ± 0.0203 | 0.2865 ± 0.1647 | 6.55 |
| 4 | 8 | 8 | 10000 | 0 | 0 | cond | 5/5 | 4.3191 ± 0.1946 | 7.9045 ± 0.9108 | 1.83 |
| 5 | 8 | 1 | 200 | 1000 | 100 | atomic | 5/5 | 0.4309 ± 0.0104 | 1.4123 ± 0.0643 | 3.28 |
| 5 | 8 | 1 | 200 | 1000 | 100 | cond | 5/5 | 0.6390 ± 0.0536 | 0.4138 ± 0.0917 | 0.64 |

## Esperas por solicitud (µs, desde antes de `entrar_*()` hasta su retorno)

Media: promedio de las medias por ejecución. Máx.: promedio de los máximos por ejecución (el mayor valor individual está en `e2.csv`).

| Caso | Variante | Lectura media | Lectura: prom. de máximos | Escritura media | Escritura: prom. de máximos | Máx. lectoras (prom.) |
|---|---|---|---|---|---|---|
| 1 | atomic | 698.7 | 1038.3 | 716.4 | 925.8 | 1.0 |
| 1 | cond | 865.2 | 17283.7 | 44.8 | 876.0 | 1.0 |
| 2 | atomic | 1385.4 | 136718.2 | 704.3 | 5730.2 | 6.0 |
| 2 | cond | 2786.3 | 124374.3 | 776.9 | 96512.2 | 6.0 |
| 3 | atomic | 2555.1 | 253623.4 | 2051.1 | 18251.3 | 2.0 |
| 3 | cond | 3036.8 | 282295.0 | 1224.8 | 244908.0 | 2.0 |
| 4 | atomic | 3.2 | 25705.7 | 1.6 | 2861.4 | 6.2 |
| 4 | cond | 427.9 | 4220413.8 | 370.8 | 14254.2 | 4.6 |
| 5 | atomic | 773.2 | 17007.5 | 1337.3 | 4693.3 | 8.0 |
| 5 | cond | 1722.6 | 98514.9 | 1243.3 | 1931.3 | 8.0 |

## Fallos de admisión del modo atómico (promedio por ejecución)

| Caso | Recurso ocupado | Escritoras esperando | CAS fallido |
|---|---|---|---|
| 1 | 24273 | 29 | 0 |
| 2 | 778610 | 18815 | 45 |
| 3 | 1476783 | 1040 | 304 |
| 4 | 60150 | 36160 | 133450 |
| 5 | 398794 | 354496 | 1563 |

## Razón atomic / cond

Valores mayores que 1 indican que la variante atómica tarda o consume más.

| Caso | Pared | CPU | Espera escritura media | Espera lectura media |
|---|---|---|---|---|
| 1 | 0.85 | 5.03 | 16.01 | 0.81 |
| 2 | 0.54 | 5.19 | 0.91 | 0.50 |
| 3 | 0.84 | 14.66 | 1.67 | 0.84 |
| 4 | 0.01 | 0.04 | 0.00 | 0.01 |
| 5 | 0.67 | 3.41 | 1.08 | 0.45 |

## Ejecuciones no válidas

Ninguna.
