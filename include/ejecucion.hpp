#pragma once

#include "configuracion.hpp"
#include "control_rw.hpp"
#include "metricas.hpp"
#include "registro.hpp"
#include <functional>
#include <string>
#include <thread>
#include <vector>

struct ResultadoEjecucion {
    long hebras_creadas = 0;
    long lecturas = 0;
    long escrituras = 0;
    Registro registro;
    long inconsistencias = 0;
    long violaciones_exclusion = 0;
    long max_lectoras = 0;
    bool diagnostico_vacio = true;
    double pared_s = 0;
    double cpu_s = 0;
    Esperas espera_lectoras;
    Esperas espera_escritoras;
    FallosAdmision fallos;
    std::vector<std::string> errores;
    bool correcto(const Configuracion& configuracion) const noexcept;
};

// Inyección utilizada para reproducir fallos de creación sin agotar recursos.
// El lanzador debe devolver una std::thread válida o lanzar antes de crearla.
using LanzadorHebras = std::function<std::thread(std::function<void()>)>;
ResultadoEjecucion ejecutar(const Configuracion& configuracion, ControlRW& control,
                           LanzadorHebras lanzador = {});
