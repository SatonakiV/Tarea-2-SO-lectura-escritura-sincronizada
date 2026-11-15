#include "configuracion.hpp"
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
long numero(std::string_view texto, const char* campo) {
    long valor = 0;
    if (texto.empty() || texto.find_first_not_of("0123456789") != texto.npos) {
        throw std::invalid_argument(std::string(campo) + ": se requiere un entero decimal sin signo");
    }
    const auto resultado = std::from_chars(texto.data(), texto.data() + texto.size(), valor);
    if (resultado.ec != std::errc{} || resultado.ptr != texto.data() + texto.size()) {
        throw std::invalid_argument(std::string(campo) + ": valor fuera del rango de long");
    }
    return valor;
}
}

void validar_configuracion(const Configuracion& c) {
    if (c.modo != Modo::Cond && c.modo != Modo::Atomic) {
        throw std::invalid_argument("modo desconocido");
    }
    if (c.politica != Politica::Lectores && c.politica != Politica::Escritores &&
        c.politica != Politica::Fifo) {
        throw std::invalid_argument("politica desconocida");
    }
    if (c.modo == Modo::Atomic && c.politica != Politica::Escritores) {
        throw std::invalid_argument("atomic solo admite la politica escritores");
    }
    // Comprobar cada operando antes de sumarlos evita desbordar incluso con
    // configuraciones construidas directamente, sin pasar por el parser.
    if (c.lectores < 1 || c.escritores < 1 || c.lectores > 1000 ||
        c.escritores > 1000 || c.lectores + c.escritores > 1000) {
        throw std::invalid_argument("se requiere L >= 1, E >= 1 y L + E <= 1000");
    }
    if (c.operaciones < 1 || c.dur_lec_us < 0 || c.dur_esc_us < 0) {
        throw std::invalid_argument("se requiere K >= 1 y duraciones >= 0");
    }
    const long limite = std::numeric_limits<long>::max();
    // Dividir antes de multiplicar: ninguna comprobación puede desbordar.
    if (c.operaciones > limite / c.escritores / 2) {
        throw std::invalid_argument("2*E*K no cabe en long");
    }
    if (c.operaciones > limite / (c.lectores + c.escritores)) {
        throw std::invalid_argument("(L+E)*K no cabe en long");
    }
}

Configuracion analizar_argumentos(const std::vector<std::string_view>& a) {
    if (a.size() != 7 && a.size() != 8) {
        throw std::invalid_argument("cantidad de argumentos incorrecta");
    }
    Configuracion c;
    if (a[0] == "cond") c.modo = Modo::Cond;
    else if (a[0] == "atomic") c.modo = Modo::Atomic;
    else throw std::invalid_argument("modo debe ser cond o atomic");

    if (a[1] == "lectores") c.politica = Politica::Lectores;
    else if (a[1] == "escritores") c.politica = Politica::Escritores;
    else if (a[1] == "fifo") c.politica = Politica::Fifo;
    else throw std::invalid_argument("politica debe ser lectores, escritores o fifo");

    c.lectores = numero(a[2], "L");
    c.escritores = numero(a[3], "E");
    c.operaciones = numero(a[4], "K");
    c.dur_lec_us = numero(a[5], "dur_lec_us");
    c.dur_esc_us = numero(a[6], "dur_esc_us");
    if (a.size() == 8) {
        if (a[7] != "notraza") throw std::invalid_argument("opcion desconocida: se esperaba notraza");
        c.traza = false;
    }
    validar_configuracion(c);
    return c;
}

const char* nombre(Modo m) { return m == Modo::Cond ? "cond" : "atomic"; }
const char* nombre(Politica p) {
    switch (p) {
    case Politica::Lectores: return "lectores";
    case Politica::Escritores: return "escritores";
    case Politica::Fifo: return "fifo";
    }
    return "desconocida";
}
const char* uso() {
    return "rw <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]";
}
