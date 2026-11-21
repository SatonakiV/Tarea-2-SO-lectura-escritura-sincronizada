#pragma once

#include "control_rw.hpp"
#include <atomic>
#include <cstdint>

// Prioridad de escritores con una sola palabra atómica y espera activa.
// Formato de la palabra (bits no usados en 0):
//   bits  0-15: lectoras activas
//   bit     16: escritora activa
//   bits 17-32: escritoras esperando
// Con L + E <= 1000 cada contador vale a lo más 999 < 2^16.
class ControlAtomicoRW : public ControlRW {
public:
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

    static constexpr std::uint64_t UNA_LECTORA = 1;
    static constexpr std::uint64_t MASCARA_LECTORAS = 0xFFFF;
    static constexpr std::uint64_t ESCRITORA_ACTIVA = std::uint64_t{1} << 16;
    static constexpr std::uint64_t UNA_ESPERANDO = std::uint64_t{1} << 17;
    static constexpr std::uint64_t MASCARA_ESPERANDO = MASCARA_LECTORAS << 17;

    struct Estado {
        long lectoras_activas;
        bool escritora_activa;
        long escritoras_esperando;
    };

    void entrar_lectura() noexcept override;
    void salir_lectura() noexcept override;
    void entrar_escritura() noexcept override;
    void salir_escritura() noexcept override;

    // Solo para pruebas y diagnóstico; nunca decide la admisión.
    Estado observar() const noexcept { return decodificar(estado_.load()); }
    static Estado decodificar(std::uint64_t palabra) noexcept;

private:
    std::atomic<std::uint64_t> estado_{0};
};
