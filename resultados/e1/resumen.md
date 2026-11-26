# E1: corrección y prioridades

Checkout `9d314d9364d665f027a5ff286d7c47e5dd5c8fc4` (rama `rama.joaco`), huella C++ `d8a48c0f37d3d1cf`, huella scripts `d96402c5f1f9524b`, 2026-11-26T08:45:40, Intel(R) Core(TM) i5-10210U CPU @ 1.60GHz (8 CPU lógicas).

**Ejecuciones correctas: 100/100.**

Evidencia: indicador del verificador (min-máx en las repeticiones). Una evidencia igual a cero no demuestra un error: la regla pudo no ponerse a prueba en esa ejecución. El modo atómico no tiene traza; se informan sus fallos de admisión por causa (promedio).

| Caso | L | E | K | lec µs | esc µs | Variante | OK | Evidencia | Máx. lectoras | Fallos ocupado | Fallos esc. esperando | Fallos CAS |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 1 | 1 | 20 | 500 | 500 | cond lectores | 5/5 | 19-19 | 1 | NA | NA | NA |
| 1 | 1 | 1 | 20 | 500 | 500 | cond escritores | 5/5 | 1-1 | 1 | NA | NA | NA |
| 1 | 1 | 1 | 20 | 500 | 500 | cond fifo | 5/5 | 0-0 | 1 | NA | NA | NA |
| 1 | 1 | 1 | 20 | 500 | 500 | atomic escritores | 5/5 | NA | 1 | 25899 | 14 | 0 |
| 2 | 6 | 2 | 100 | 200 | 500 | cond lectores | 5/5 | 586-592 | 6 | NA | NA | NA |
| 2 | 6 | 2 | 100 | 200 | 500 | cond escritores | 5/5 | 124-149 | 6 | NA | NA | NA |
| 2 | 6 | 2 | 100 | 200 | 500 | cond fifo | 5/5 | 394-402 | 6 | NA | NA | NA |
| 2 | 6 | 2 | 100 | 200 | 500 | atomic escritores | 5/5 | NA | 6 | 801665 | 14064 | 34 |
| 3 | 2 | 6 | 100 | 500 | 200 | cond lectores | 5/5 | 198-198 | 2 | NA | NA | NA |
| 3 | 2 | 6 | 100 | 500 | 200 | cond escritores | 5/5 | 3-32 | 2 | NA | NA | NA |
| 3 | 2 | 6 | 100 | 500 | 200 | cond fifo | 5/5 | 0-100 | 2 | NA | NA | NA |
| 3 | 2 | 6 | 100 | 500 | 200 | atomic escritores | 5/5 | NA | 2 | 1330079 | 1324 | 323 |
| 4 | 8 | 8 | 100 | 0 | 0 | cond lectores | 5/5 | 0-259 | 2 | NA | NA | NA |
| 4 | 8 | 8 | 100 | 0 | 0 | cond escritores | 5/5 | 0-20 | 2 | NA | NA | NA |
| 4 | 8 | 8 | 100 | 0 | 0 | cond fifo | 5/5 | 172-472 | 2 | NA | NA | NA |
| 4 | 8 | 8 | 100 | 0 | 0 | atomic escritores | 5/5 | NA | 1 | 0 | 0 | 0 |
| 5 | 8 | 1 | 200 | 1000 | 100 | cond lectores | 5/5 | 1588-1588 | 8 | NA | NA | NA |
| 5 | 8 | 1 | 200 | 1000 | 100 | cond escritores | 5/5 | 226-255 | 8 | NA | NA | NA |
| 5 | 8 | 1 | 200 | 1000 | 100 | cond fifo | 5/5 | 1384-1389 | 8 | NA | NA | NA |
| 5 | 8 | 1 | 200 | 1000 | 100 | atomic escritores | 5/5 | NA | 8 | 422796 | 455186 | 1556 |

## Errores para remitir

Ninguno.
