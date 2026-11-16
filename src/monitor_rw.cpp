#include "monitor_rw.hpp"
#include <cassert>
#include <stdexcept>

MonitorRW::MonitorRW(Politica politica, ObservadorAdmision* observador)
    : politica_(politica), observador_(observador) {
    if (politica != Politica::Lectores && politica != Politica::Escritores) {
        throw std::invalid_argument("FIFO pendiente de integrar por Persona 2");
    }
}

void MonitorRW::comprobar_invariantes() const noexcept {
    assert(lectoras_activas_ >= 0);
    assert(lectoras_pendientes_ >= 0);
    assert(escritoras_pendientes_ >= 0);
    assert(!escritora_activa_ || lectoras_activas_ == 0);
}

void MonitorRW::anotar(Tipo tipo, Accion accion) noexcept {
    if (observador_) observador_->registrar({id_hebra, tipo, accion, -1});
}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m_);
    comprobar_invariantes();
    anotar(Tipo::Lectora, Accion::Pide);
    ++lectoras_pendientes_;
    comprobar_invariantes();
    try {
        lectoras_.wait(lock, [this] {
            return !escritora_activa_ &&
                (politica_ == Politica::Lectores || escritoras_pendientes_ == 0);
        });
    } catch (...) {
        --lectoras_pendientes_;
        comprobar_invariantes();
        notificar_habilitados();
        throw;
    }
    assert(!escritora_activa_);
    assert(politica_ == Politica::Lectores || escritoras_pendientes_ == 0);
    --lectoras_pendientes_;
    ++lectoras_activas_;
    comprobar_invariantes();
    anotar(Tipo::Lectora, Accion::Entra);
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lock(m_);
    comprobar_invariantes();
    assert(lectoras_activas_ > 0 && !escritora_activa_);
    --lectoras_activas_;
    comprobar_invariantes();
    anotar(Tipo::Lectora, Accion::Sale);
    notificar_habilitados();
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lock(m_);
    comprobar_invariantes();
    anotar(Tipo::Escritora, Accion::Pide);
    ++escritoras_pendientes_;
    comprobar_invariantes();
    try {
        escritoras_.wait(lock, [this] {
            return !escritora_activa_ && lectoras_activas_ == 0 &&
                (politica_ == Politica::Escritores || lectoras_pendientes_ == 0);
        });
    } catch (...) {
        --escritoras_pendientes_;
        comprobar_invariantes();
        notificar_habilitados();
        throw;
    }
    assert(!escritora_activa_ && lectoras_activas_ == 0);
    assert(politica_ == Politica::Escritores || lectoras_pendientes_ == 0);
    --escritoras_pendientes_;
    escritora_activa_ = true;
    comprobar_invariantes();
    anotar(Tipo::Escritora, Accion::Entra);
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lock(m_);
    comprobar_invariantes();
    assert(escritora_activa_ && lectoras_activas_ == 0);
    escritora_activa_ = false;
    comprobar_invariantes();
    anotar(Tipo::Escritora, Accion::Sale);
    notificar_habilitados();
}

void MonitorRW::notificar_habilitados() noexcept {
    if (escritora_activa_) return;
    if (politica_ == Politica::Lectores) {
        if (lectoras_pendientes_ > 0) lectoras_.notify_all();
        else if (lectoras_activas_ == 0 && escritoras_pendientes_ > 0) escritoras_.notify_one();
    } else {
        if (escritoras_pendientes_ > 0) {
            if (lectoras_activas_ == 0) escritoras_.notify_one();
        } else if (lectoras_pendientes_ > 0) {
            lectoras_.notify_all();
        }
    }
}
