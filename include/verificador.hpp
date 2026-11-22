#pragma once

#include "tipos.hpp"
#include <cstddef>
#include <map>
#include <string>
#include <vector>

enum class FaseSolicitud {
    Pendiente,
    Activa
};

struct Solicitud {
    Tipo tipo;
    FaseSolicitud fase;
    long turno;
};

struct EstadoTraza {
    int lectoras_pendientes = 0;
    int escritoras_pendientes = 0;
    int lectoras_activas = 0;
    int escritoras_activas = 0;
    std::map<int, Solicitud> solicitudes;
};

struct ResultadoVerificacion {
    bool valida = true;
    std::size_t indice = 0;
    std::string error;
    long evidencia = 0;
};

bool aplicar_evento(
    EstadoTraza& estado,
    const AnotacionTraza& evento,
    std::string& error
);

ResultadoVerificacion verificar_traza(
    const std::vector<AnotacionTraza>& traza
);

ResultadoVerificacion verificar_traza(
    const std::vector<AnotacionTraza>& traza,
    Politica politica
);
