#include "control_atomico_rw.hpp"
#include "metricas.hpp"
#include <cassert>
#include <thread>

ControlAtomicoRW::Estado ControlAtomicoRW::decodificar(std::uint64_t p) noexcept {
    return {static_cast<long>(p & MASCARA_LECTORAS), (p & ESCRITORA_ACTIVA) != 0,
            static_cast<long>((p & MASCARA_ESPERANDO) >> 17)};
}

void ControlAtomicoRW::entrar_lectura() noexcept {
    std::uint64_t esperado = estado_.load();
    for (;;) {
        // Un fallo por intento; la escritora activa tiene prioridad como causa.
        if (esperado & ESCRITORA_ACTIVA) {
            incrementar_fallo(fallos_hebra.recurso_ocupado, fallos_hebra.saturado);
        } else if (esperado & MASCARA_ESPERANDO) {
            incrementar_fallo(fallos_hebra.escritoras_esperando, fallos_hebra.saturado);
        } else if (estado_.compare_exchange_weak(esperado, esperado + UNA_LECTORA)) {
            return;
        } else {
            // esperado ya trae el valor actual.
            incrementar_fallo(fallos_hebra.cas_fallido, fallos_hebra.saturado);
            continue;
        }
        std::this_thread::yield();
        esperado = estado_.load();
    }
}

void ControlAtomicoRW::salir_lectura() noexcept {
    const std::uint64_t previo = estado_.fetch_sub(UNA_LECTORA);
    assert((previo & MASCARA_LECTORAS) > 0 && !(previo & ESCRITORA_ACTIVA));
    (void)previo;
}

void ControlAtomicoRW::entrar_escritura() noexcept {
    // Desde aquí ninguna lectora nueva puede entrar.
    std::uint64_t esperado = estado_.fetch_add(UNA_ESPERANDO) + UNA_ESPERANDO;
    for (;;) {
        if ((esperado & ESCRITORA_ACTIVA) || (esperado & MASCARA_LECTORAS)) {
            incrementar_fallo(fallos_hebra.recurso_ocupado, fallos_hebra.saturado);
        } else if (estado_.compare_exchange_weak(esperado,
                                                 esperado - UNA_ESPERANDO + ESCRITORA_ACTIVA)) {
            return;
        } else {
            incrementar_fallo(fallos_hebra.cas_fallido, fallos_hebra.saturado);
            continue;
        }
        std::this_thread::yield();
        esperado = estado_.load();
    }
}

void ControlAtomicoRW::salir_escritura() noexcept {
    const std::uint64_t previo = estado_.fetch_sub(ESCRITORA_ACTIVA);
    assert((previo & ESCRITORA_ACTIVA) && (previo & MASCARA_LECTORAS) == 0);
    (void)previo;
}
