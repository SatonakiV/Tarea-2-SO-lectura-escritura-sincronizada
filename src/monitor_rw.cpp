#include "monitor_rw.hpp"
#include <cassert>
#include <exception>
#include <stdexcept>

MonitorRW::MonitorRW(Politica politica, ObservadorAdmision* observador, bool habilitar_traza)
    : politica_(politica), traza_habilitada_(habilitar_traza), observador_(observador) {
    if (politica != Politica::Lectores &&
        politica != Politica::Escritores &&
        politica != Politica::Fifo) {
        throw std::invalid_argument("Politica desconocida");
    }
}

void MonitorRW::comprobar_invariantes() const noexcept {
    assert(lectoras_activas_ >= 0);
    assert(lectoras_pendientes_ >= 0);
    assert(escritoras_pendientes_ >= 0);
    assert(!escritora_activa_ || lectoras_activas_ == 0);
    assert(turno_por_admitir_ >= 0);
    assert(turno_por_admitir_ <= siguiente_turno_);
}

void MonitorRW::anotar(Tipo tipo, Accion accion, long turno) noexcept {
    const AnotacionTraza evento{id_hebra, tipo, accion, turno};

    if (traza_habilitada_ && !traza_incompleta_) {
        try {
            traza_.push_back(evento);
        } catch (...) {
            traza_incompleta_ = true;
        }
    }

    if (observador_) observador_->registrar(evento);
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::lock_guard<std::mutex> lock(m_);

    if (!traza_habilitada_) return {};

    if (traza_incompleta_) {
        throw std::runtime_error("La traza esta incompleta por un error de memoria");
    }

    return traza_;
}

void MonitorRW::saltar_turnos_cancelados() noexcept {
    while (true) {
        auto it = turnos_cancelados_.find(turno_por_admitir_);

        if (it == turnos_cancelados_.end()) break;

        turnos_cancelados_.erase(it);
        ++turno_por_admitir_;
    }
}

void MonitorRW::cancelar_turno(long turno) {
    if (politica_ != Politica::Fifo) return;

    if (turno == turno_por_admitir_) {
        ++turno_por_admitir_;
        saltar_turnos_cancelados();
    } else if (turno > turno_por_admitir_) {
        try {
            turnos_cancelados_.insert(turno);
        } catch (...) {
            std::terminate();
        }
    }
}

bool MonitorRW::conservar_turnos_activos() const noexcept {
    return politica_ == Politica::Fifo &&
           (traza_habilitada_ || observador_ != nullptr);
}

long MonitorRW::retirar_turno_activo() noexcept {
    if (!conservar_turnos_activos()) return -1;

    auto it = turnos_activos_.find(std::this_thread::get_id());

    if (it == turnos_activos_.end()) std::terminate();

    const long turno = it->second;
    turnos_activos_.erase(it);
    return turno;
}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m_);
    comprobar_invariantes(); // Comprobamos que el estado del monitor sea valido antes de registrar una nueva solicitud de lectura.

    const long mi_turno =
        politica_ == Politica::Fifo ? siguiente_turno_++ : -1;

    anotar(Tipo::Lectora, Accion::Pide, mi_turno);
    ++lectoras_pendientes_;
    comprobar_invariantes(); // Comprobamos que los contadores y turnos sigan siendo validos despues de registrar la solicitud.

    try {
        lectoras_.wait(lock, [this, mi_turno] {
            // Verifica todas las condiciones y retorna si es que puede entrar a leer, en caso de que cumpla cada una de ellas.
            // Esto depende de la politica de acceso establecida, incluyendo el respeto del turno en FIFO.
            return !escritora_activa_ &&
                (politica_ == Politica::Lectores ||
                 (politica_ == Politica::Escritores &&
                  escritoras_pendientes_ == 0) ||
                 (politica_ == Politica::Fifo &&
                  mi_turno == turno_por_admitir_));
        });

        if (conservar_turnos_activos()) {
            const auto insertado =
                turnos_activos_.emplace(std::this_thread::get_id(), mi_turno);

            if (!insertado.second) {
                throw std::logic_error("La hebra ya tiene un turno activo");
            }
        }
    } catch (...) {
        --lectoras_pendientes_;
        cancelar_turno(mi_turno);
        comprobar_invariantes(); // Comprobamos que el estado del monitor siga siendo valido despues de retirar la solicitud pendiente por un error.
        notificar_habilitados();
        throw;
    }

    assert(!escritora_activa_);
    assert(politica_ == Politica::Lectores ||
           (politica_ == Politica::Escritores &&
            escritoras_pendientes_ == 0) ||
           (politica_ == Politica::Fifo &&
            mi_turno == turno_por_admitir_));

    --lectoras_pendientes_;
    ++lectoras_activas_;

    if (politica_ == Politica::Fifo) {
        ++turno_por_admitir_;
        saltar_turnos_cancelados();
    }

    comprobar_invariantes(); // Comprobamos que el estado siga siendo valido despues de admitir a la lectora y avanzar el turno FIFO.
    anotar(Tipo::Lectora, Accion::Entra, mi_turno);

    if (politica_ == Politica::Fifo) {
        notificar_habilitados();
    }
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lock(m_);
    comprobar_invariantes(); // Comprobamos que el estado del monitor sea valido antes de registrar la salida de la lectora.

    assert(lectoras_activas_ > 0 && !escritora_activa_);

    const long mi_turno = retirar_turno_activo();

    --lectoras_activas_;
    comprobar_invariantes(); // Comprobamos que el contador de lectoras activas siga siendo valido despues de registrar su salida.

    anotar(Tipo::Lectora, Accion::Sale, mi_turno);
    notificar_habilitados();
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lock(m_);
    comprobar_invariantes(); // Comprobamos que el estado del monitor sea valido antes de registrar una nueva solicitud de escritura.

    const long mi_turno =
        politica_ == Politica::Fifo ? siguiente_turno_++ : -1;

    anotar(Tipo::Escritora, Accion::Pide, mi_turno);
    ++escritoras_pendientes_;
    comprobar_invariantes(); // Comprobamos que los contadores y turnos sigan siendo validos despues de registrar la solicitud.

    try {
        escritoras_.wait(lock, [this, mi_turno] {
            // Verifica todas las condiciones y retorna si es que puede entrar a escribir, en caso de que cumpla cada una de ellas.
            // Esto depende de la politica de acceso establecida, incluyendo el respeto del turno en FIFO.
            return escritora_activa_ == false &&
                   lectoras_activas_ == 0 &&
                (politica_ == Politica::Escritores ||
                 (politica_ == Politica::Lectores &&
                  lectoras_pendientes_ == 0) ||
                 (politica_ == Politica::Fifo &&
                  mi_turno == turno_por_admitir_));
        });

        if (conservar_turnos_activos()) {
            const auto insertado =
                turnos_activos_.emplace(std::this_thread::get_id(), mi_turno);

            if (!insertado.second) {
                throw std::logic_error("La hebra ya tiene un turno activo");
            }
        }
    } catch (...) {
        --escritoras_pendientes_;
        cancelar_turno(mi_turno);
        comprobar_invariantes(); // Comprobamos que el estado del monitor siga siendo valido despues de retirar la solicitud pendiente por un error.
        notificar_habilitados();
        throw;
    }

    assert(!escritora_activa_ && lectoras_activas_ == 0);
    assert(politica_ == Politica::Escritores ||
           (politica_ == Politica::Lectores &&
            lectoras_pendientes_ == 0) ||
           (politica_ == Politica::Fifo &&
            mi_turno == turno_por_admitir_));

    --escritoras_pendientes_;
    escritora_activa_ = true;

    if (politica_ == Politica::Fifo) {
        ++turno_por_admitir_;
        saltar_turnos_cancelados();
    }

    comprobar_invariantes(); // Comprobamos que la escritora haya sido admitida sin violar la exclusion mutua ni el orden de los turnos.
    anotar(Tipo::Escritora, Accion::Entra, mi_turno);
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lock(m_);
    comprobar_invariantes(); // Comprobamos que el estado del monitor sea valido antes de registrar la salida de la escritora.

    assert(escritora_activa_ && lectoras_activas_ == 0);

    const long mi_turno = retirar_turno_activo();

    escritora_activa_ = false;
    comprobar_invariantes(); // Comprobamos que el estado siga siendo valido despues de indicar que ya no hay una escritora activa.

    anotar(Tipo::Escritora, Accion::Sale, mi_turno);
    notificar_habilitados();
}

void MonitorRW::notificar_habilitados() noexcept {
    if (escritora_activa_) return;

    if (politica_ == Politica::Fifo) {
        if (lectoras_pendientes_ > 0) {
            lectoras_.notify_all();
        }
        if (escritoras_pendientes_ > 0) {
            escritoras_.notify_all();
        }
    } else if (politica_ == Politica::Lectores) {
        if (lectoras_pendientes_ > 0) {
            lectoras_.notify_all();
        } else if (lectoras_activas_ == 0 &&
                   escritoras_pendientes_ > 0) {
            escritoras_.notify_one();
        }
    } else {
        if (escritoras_pendientes_ > 0) {
            if (lectoras_activas_ == 0) {
                escritoras_.notify_one();
            }
        } else if (lectoras_pendientes_ > 0) {
            lectoras_.notify_all();
        }
    }
}
