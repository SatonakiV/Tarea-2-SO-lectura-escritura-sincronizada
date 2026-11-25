# E1: corrección y prioridades

Commit `b9e8e06bdb2b5b002c02c34194eb2a7c6839e134` (rama `rama.joaco`), 2026-11-25T20:31:21, Intel(R) Core(TM) i5-10210U CPU @ 1.60GHz (8 CPU lógicas).

**Ejecuciones correctas: 100/100.**

Evidencia: indicador del verificador (min-máx en las repeticiones). Una evidencia igual a cero no demuestra un error: la regla pudo no ponerse a prueba en esa ejecución. El modo atómico no tiene traza; se informan sus fallos de admisión por causa (promedio).

| Caso | L | E | K | lec µs | esc µs | Variante | OK | Evidencia | Máx. lectoras | Fallos ocupado | Fallos esc. esperando | Fallos CAS |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 1 | 1 | 20 | 500 | 500 | cond lectores | 5/5 | 19-19 | 1 | NA | NA | NA |
| 1 | 1 | 1 | 20 | 500 | 500 | cond escritores | 5/5 | 1-1 | 1 | NA | NA | NA |
| 1 | 1 | 1 | 20 | 500 | 500 | cond fifo | 5/5 | 0-0 | 1 | NA | NA | NA |
| 1 | 1 | 1 | 20 | 500 | 500 | atomic escritores | 5/5 | NA | 1 | 26021 | 312 | 0 |
| 2 | 6 | 2 | 100 | 200 | 500 | cond lectores | 5/5 | 587-591 | 6 | NA | NA | NA |
| 2 | 6 | 2 | 100 | 200 | 500 | cond escritores | 5/5 | 47-134 | 6 | NA | NA | NA |
| 2 | 6 | 2 | 100 | 200 | 500 | cond fifo | 5/5 | 390-487 | 6 | NA | NA | NA |
| 2 | 6 | 2 | 100 | 200 | 500 | atomic escritores | 5/5 | NA | 6 | 1133309 | 181675 | 102 |
| 3 | 2 | 6 | 100 | 500 | 200 | cond lectores | 5/5 | 198-198 | 2 | NA | NA | NA |
| 3 | 2 | 6 | 100 | 500 | 200 | cond escritores | 5/5 | 4-59 | 2 | NA | NA | NA |
| 3 | 2 | 6 | 100 | 500 | 200 | cond fifo | 5/5 | 1-100 | 2 | NA | NA | NA |
| 3 | 2 | 6 | 100 | 500 | 200 | atomic escritores | 5/5 | NA | 2 | 3096942 | 3328 | 222 |
| 4 | 8 | 8 | 100 | 0 | 0 | cond lectores | 5/5 | 0-0 | 1 | NA | NA | NA |
| 4 | 8 | 8 | 100 | 0 | 0 | cond escritores | 5/5 | 0-2 | 2 | NA | NA | NA |
| 4 | 8 | 8 | 100 | 0 | 0 | cond fifo | 5/5 | 0-560 | 2 | NA | NA | NA |
| 4 | 8 | 8 | 100 | 0 | 0 | atomic escritores | 5/5 | NA | 2 | 0 | 0 | 6 |
| 5 | 8 | 1 | 200 | 1000 | 100 | cond lectores | 5/5 | 1587-1592 | 8 | NA | NA | NA |
| 5 | 8 | 1 | 200 | 1000 | 100 | cond escritores | 5/5 | 191-264 | 8 | NA | NA | NA |
| 5 | 8 | 1 | 200 | 1000 | 100 | cond fifo | 5/5 | 1398-1400 | 8 | NA | NA | NA |
| 5 | 8 | 1 | 200 | 1000 | 100 | atomic escritores | 5/5 | NA | 8 | 1771017 | 2533287 | 557 |

## Errores para remitir

Ninguno.
