#include "control_atomico_rw.hpp"
#include "integraciones.hpp"

std::unique_ptr<ControlRW> crear_atomico() {
    return std::make_unique<ControlAtomicoRW>();
}
