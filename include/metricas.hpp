#pragma once
#include "tipos.hpp"
#include <atomic>
#include <cstdint>

// P3 incrementa fallos_hebra dentro de su protocolo; cada hebra es su única
// escritora. El ejecutor copia y suma los valores únicamente tras los join().
struct FallosAdmision {
    std::uint64_t recurso_ocupado = 0;
    std::uint64_t escritoras_esperando = 0;
    std::uint64_t cas_fallido = 0;
    bool saturado = false;
};
inline thread_local FallosAdmision fallos_hebra;
void incrementar_fallo(std::uint64_t& contador, bool& saturado) noexcept;
void acumular_fallos(FallosAdmision& destino, const FallosAdmision& origen) noexcept;

struct Esperas {
    long solicitudes = 0;
    long double suma_ns = 0;
    std::uint64_t max_ns = 0;
    void agregar(std::uint64_t ns) noexcept;
    void acumular(const Esperas& otra) noexcept;
    long double media_us() const noexcept;
};

// Estos atómicos OBSERVAN la admisión; nunca deciden si una hebra puede entrar.
// El orden secuencial evita perder ambos lados de un solapamiento al comprobar
// los dos contadores. El CAS siguiente solo mantiene un máximo diagnóstico.
class Diagnostico {
public:
    void entrar(Tipo tipo) noexcept;
    void salir(Tipo tipo) noexcept;
    long max_lectoras() const noexcept;
    long violaciones() const noexcept;
    bool vacio() const noexcept;
private:
    std::atomic<long> lectoras_{0};
    std::atomic<long> escritoras_{0};
    std::atomic<long> max_lectoras_{0};
    std::atomic<long> violaciones_{0};
};

double cpu_proceso_segundos();
const char* fuente_cpu() noexcept;
