#include "integraciones.hpp"

std::unique_ptr<ControlRW> crear_control(const Configuracion& c) {
    validar_configuracion(c);
    return c.modo == Modo::Cond ? crear_monitor(c.politica, c.traza) : crear_atomico();
}
