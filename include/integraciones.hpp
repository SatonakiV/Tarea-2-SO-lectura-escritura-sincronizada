#pragma once
#include "configuracion.hpp"
#include "control_rw.hpp"
#include <cstddef>
#include <memory>
#include <string>

enum class EstadoPolitica { Pendiente, Desactivada, Correcta, Incorrecta };
struct ResultadoPolitica {
    EstadoPolitica estado = EstadoPolitica::Pendiente;
    long evidencia = 0;
    // Solo significativos si estado == Incorrecta; indice cuenta desde cero.
    std::size_t indice = 0;
    std::string regla_incumplida{};
};

// P2 mantiene estas dos funciones en integracion_monitor.cpp. La segunda se
// llama DESPUÉS de todos los join(), fuera de la región cronometrada.
std::unique_ptr<ControlRW> crear_monitor(Politica politica, bool traza);
ResultadoPolitica verificar_politica(ControlRW& control, Politica politica, bool traza);
// P3 sustituye el rechazo en integracion_atomico.cpp por su controlador.
std::unique_ptr<ControlRW> crear_atomico();
// Punto único de selección; no contiene carga de trabajo ni métricas.
std::unique_ptr<ControlRW> crear_control(const Configuracion& configuracion);
