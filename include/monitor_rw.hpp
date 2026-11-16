#pragma once

#include "control_rw.hpp"
#include "tipos.hpp"
#include <condition_variable>
#include <mutex>

class MonitorRW : public ControlRW {
public:
    explicit MonitorRW(Politica politica, ObservadorAdmision* observador = nullptr);
    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;

private:
    void comprobar_invariantes() const noexcept;   // requiere m adquirido
    void anotar(Tipo tipo, Accion accion) noexcept; // requiere m adquirido
    void notificar_habilitados() noexcept;         // requiere m adquirido

    const Politica politica_;
    std::mutex m_;
    std::condition_variable lectoras_;
    std::condition_variable escritoras_;
    long lectoras_activas_ = 0;
    long lectoras_pendientes_ = 0;
    long escritoras_pendientes_ = 0;
    bool escritora_activa_ = false;
    ObservadorAdmision* const observador_;
};
