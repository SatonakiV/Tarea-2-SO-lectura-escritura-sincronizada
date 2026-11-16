#pragma once

#include "tipos.hpp"
#include <string_view>
#include <vector>

struct Configuracion {
    Modo modo = Modo::Cond;
    Politica politica = Politica::Lectores;
    long lectores = 1;
    long escritores = 1;
    long operaciones = 1;
    long dur_lec_us = 0;
    long dur_esc_us = 0;
    bool traza = true;
};

// Los argumentos excluyen argv[0]. Lanza invalid_argument ante un error.
Configuracion analizar_argumentos(const std::vector<std::string_view>& argumentos);
void validar_configuracion(const Configuracion& configuracion);
const char* nombre(Modo modo);
const char* nombre(Politica politica);
const char* uso();
