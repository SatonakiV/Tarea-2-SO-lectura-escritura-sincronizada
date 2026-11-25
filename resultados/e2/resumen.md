# E2: costo de sincronización

Commit `b9e8e06bdb2b5b002c02c34194eb2a7c6839e134` (rama `rama.joaco`), 2026-11-25T20:31:54, Intel(R) Core(TM) i5-10210U CPU @ 1.60GHz (8 CPU lógicas), g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0.

Promedio de las repeticiones (± desviación estándar), `notraza`. Pared: `steady_clock`; CPU: `CLOCK_PROCESS_CPUTIME_ID`. Ambas cubren desde antes de crear las hebras hasta completar todos los `join()`. El tiempo de CPU incluye trabajo útil, no solo espera.

## Tiempos

| Caso | L | E | K | lec µs | esc µs | Variante | Válidas | Pared (s) | CPU (s) | CPU/pared |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 1 | 1 | 20 | 500 | 500 | atomic | 5/5 | 0.0308 ± 0.0027 | 0.0326 ± 0.0035 | 1.06 |
| 1 | 1 | 1 | 20 | 500 | 500 | cond | 5/5 | 0.0335 ± 0.0034 | 0.0065 ± 0.0022 | 0.19 |
| 2 | 6 | 2 | 100 | 200 | 500 | atomic | 5/5 | 0.2191 ± 0.0499 | 1.2853 ± 0.2909 | 5.87 |
| 2 | 6 | 2 | 100 | 200 | 500 | cond | 5/5 | 0.2868 ± 0.0429 | 0.1741 ± 0.0585 | 0.59 |
| 3 | 2 | 6 | 100 | 500 | 200 | atomic | 5/5 | 0.3998 ± 0.1184 | 2.2463 ± 0.8390 | 5.53 |
| 3 | 2 | 6 | 100 | 500 | 200 | cond | 5/5 | 0.3232 ± 0.0413 | 0.0795 ± 0.0269 | 0.24 |
| 4 | 8 | 8 | 10000 | 0 | 0 | atomic | 5/5 | 0.0424 ± 0.0095 | 0.2617 ± 0.0783 | 6.22 |
| 4 | 8 | 8 | 10000 | 0 | 0 | cond | 5/5 | 4.1483 ± 0.0951 | 7.2038 ± 0.1749 | 1.74 |
| 5 | 8 | 1 | 200 | 1000 | 100 | atomic | 5/5 | 0.5833 ± 0.1219 | 2.5929 ± 0.8637 | 4.37 |
| 5 | 8 | 1 | 200 | 1000 | 100 | cond | 5/5 | 0.6269 ± 0.0449 | 0.3998 ± 0.0944 | 0.63 |

## Esperas por solicitud (µs, desde antes de `entrar_*()` hasta su retorno)

| Caso | Variante | Lectura media | Lectura máx. | Escritura media | Escritura máx. | Máx. lectoras |
|---|---|---|---|---|---|---|
| 1 | atomic | 718.7 | 1103.4 | 740.7 | 1026.8 | 1.0 |
| 1 | cond | 863.6 | 17255.5 | 34.2 | 662.0 | 1.0 |
| 2 | atomic | 1808.2 | 148613.1 | 891.0 | 11006.8 | 6.0 |
| 2 | cond | 2419.6 | 126969.4 | 683.6 | 84618.5 | 6.0 |
| 3 | atomic | 3335.9 | 331737.6 | 2600.7 | 32712.1 | 2.0 |
| 3 | cond | 2517.2 | 218476.3 | 1046.3 | 199246.6 | 2.0 |
| 4 | atomic | 2.8 | 21412.7 | 1.4 | 3639.6 | 6.0 |
| 4 | cond | 411.5 | 4055020.8 | 356.5 | 15036.0 | 4.0 |
| 5 | atomic | 1557.5 | 32898.1 | 1721.2 | 9243.8 | 8.0 |
| 5 | cond | 1770.4 | 116991.5 | 1242.6 | 1938.4 | 8.0 |

## Fallos de admisión del modo atómico (promedio por ejecución)

| Caso | Recurso ocupado | Escritoras esperando | CAS fallido |
|---|---|---|---|
| 1 | 23976 | 57 | 0 |
| 2 | 897238 | 131420 | 69 |
| 3 | 1900913 | 1803 | 230 |
| 4 | 55770 | 32914 | 120270 |
| 5 | 911131 | 1426125 | 1000 |

## Razón atomic / cond

Valores mayores que 1 indican que la variante atómica tarda o consume más.

| Caso | Pared | CPU | Espera escritura media | Espera lectura media |
|---|---|---|---|---|
| 1 | 0.92 | 5.05 | 21.69 | 0.83 |
| 2 | 0.76 | 7.38 | 1.30 | 0.75 |
| 3 | 1.24 | 28.24 | 2.49 | 1.33 |
| 4 | 0.01 | 0.04 | 0.00 | 0.01 |
| 5 | 0.93 | 6.48 | 1.39 | 0.88 |
