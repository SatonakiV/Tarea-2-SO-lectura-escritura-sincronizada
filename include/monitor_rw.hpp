#pragma once

#include "control_rw.hpp"
#include "tipos.hpp"
#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

class MonitorRW : public ControlRW {
public:
    explicit MonitorRW(Politica politica, ObservadorAdmision* observador = nullptr,
                       bool habilitar_traza = true);
    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;
    std::vector<AnotacionTraza> obtener_traza();

private:
    void comprobar_invariantes() const noexcept;   // requiere m adquirido
    void anotar(Tipo tipo, Accion accion, long turno = -1) noexcept; // requiere m adquirido
    void notificar_habilitados() noexcept;         // requiere m adquirido
    void cancelar_turno(long turno);
    void saltar_turnos_cancelados() noexcept;
    bool conservar_turnos_activos() const noexcept;
    long retirar_turno_activo() noexcept;

    const Politica politica_;
    const bool traza_habilitada_;
    std::mutex m_;
    std::condition_variable lectoras_;
    std::condition_variable escritoras_;

    long lectoras_activas_ = 0;
    long lectoras_pendientes_ = 0;
    long escritoras_pendientes_ = 0;
    bool escritora_activa_ = false;

    long siguiente_turno_ = 0;
    long turno_por_admitir_ = 0;
    std::set<long> turnos_cancelados_;
    std::map<std::thread::id, long> turnos_activos_;

    std::vector<AnotacionTraza> traza_;
    bool traza_incompleta_ = false;
    ObservadorAdmision* const observador_;
};
