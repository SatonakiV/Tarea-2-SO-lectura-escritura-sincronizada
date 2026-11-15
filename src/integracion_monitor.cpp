#include "integraciones.hpp"
#include "monitor_rw.hpp"

std::unique_ptr<ControlRW> crear_monitor(Politica politica, bool /*traza*/) {
    // P2: extender MonitorRW con FIFO y traza privada R6; pasar aquí el flag.
    // Hoy el constructor rechaza FIFO y no se afirma que exista traza R6.
    return std::make_unique<MonitorRW>(politica);
}
ResultadoPolitica verificar_politica(ControlRW&, Politica, bool traza) {
    // P2: obtener_traza() protegido + verificar_traza(); informar índice y regla
    // al fallar. No convertir "pendiente" en un éxito del verificador.
    return {traza ? EstadoPolitica::Pendiente : EstadoPolitica::Desactivada, 0};
}
