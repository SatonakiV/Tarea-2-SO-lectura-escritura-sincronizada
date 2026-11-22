#include "integraciones.hpp"
#include "monitor_rw.hpp"
#include "verificador.hpp"
#include <exception>

std::unique_ptr<ControlRW> crear_monitor(Politica politica, bool traza) {
    return std::make_unique<MonitorRW>(politica, nullptr, traza);
}

ResultadoPolitica verificar_politica(ControlRW& control, Politica politica, bool traza) {
    if (!traza) {
        return {EstadoPolitica::Desactivada, 0};
    }

    auto* monitor = dynamic_cast<MonitorRW*>(&control);

    if (!monitor) {
        return {EstadoPolitica::Incorrecta, 0, 0, "Control sin monitor para verificar"};
    }

    try {
        const auto resultado =
            verificar_traza(monitor->obtener_traza(), politica);

        return {resultado.valida ? EstadoPolitica::Correcta : EstadoPolitica::Incorrecta,
            resultado.evidencia,
            resultado.indice,
            resultado.error
        };

    } catch (const std::exception& e) {
        return {EstadoPolitica::Incorrecta, 0, 0, e.what()};
    }
}
