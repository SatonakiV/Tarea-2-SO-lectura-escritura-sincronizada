#include "monitor_rw.hpp"
#include "integraciones.hpp"
#include "verificador.hpp"
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

void exigir(bool condicion, const char* mensaje) {
    if (!condicion) throw std::runtime_error(mensaje);
}

class Eventos : public ObservadorAdmision {
public:
    Eventos() { eventos_.reserve(32); }

    void registrar(const AnotacionTraza& e) noexcept override {
        std::lock_guard<std::mutex> lock(m_);
        try { eventos_.push_back(e); } catch (...) { perdidos_ = true; }
        cv_.notify_all();
    }

    bool esperar(int id, Accion accion) {
        std::unique_lock<std::mutex> lock(m_);
        return cv_.wait_for(lock, std::chrono::seconds(5), [&] {
            return perdidos_ || contiene(id, accion);
        }) && !perdidos_;
    }

    std::vector<AnotacionTraza> copia() {
        std::lock_guard<std::mutex> lock(m_);
        exigir(!perdidos_, "eventos perdidos");
        return eventos_;
    }

private:
    bool contiene(int id, Accion accion) const {
        for (const auto& e : eventos_)
            if (e.hebra == id && e.accion == accion) return true;
        return false;
    }

    std::mutex m_;
    std::condition_variable cv_;
    std::vector<AnotacionTraza> eventos_;
    bool perdidos_ = false;
};

class Puerta {
public:
    void esperar() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return abierta_; });
    }

    void abrir() {
        std::lock_guard<std::mutex> lock(m_);
        abierta_ = true;
        cv_.notify_all();
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    bool abierta_ = false;
};

std::size_t indice(
    const std::vector<AnotacionTraza>& eventos,
    int id,
    Accion accion
) {
    for (std::size_t i = 0; i < eventos.size(); ++i)
        if (eventos[i].hebra == id && eventos[i].accion == accion) return i;

    throw std::runtime_error("evento esperado ausente");
}

bool contiene(
    const std::vector<AnotacionTraza>& eventos,
    int id,
    Accion accion
) {
    for (const auto& e : eventos)
        if (e.hebra == id && e.accion == accion) return true;

    return false;
}

void comprobar_traza(
    MonitorRW& monitor,
    Politica politica,
    std::size_t solicitudes
) {
    const auto traza = monitor.obtener_traza();

    exigir(traza.size() == solicitudes * 3,
           "cantidad de eventos incorrecta");

    const auto resultado = verificar_traza(traza, politica);

    if (!resultado.valida) {
        throw std::runtime_error(resultado.error);
    }

    if (politica == Politica::Fifo) {
        long turno = 0;

        for (const auto& evento : traza) {
            if (evento.accion == Accion::Pide) {
                exigir(evento.turno == turno++,
                       "turnos de solicitud incorrectos");
            }
        }
    }
}

void prueba_lectoras_consecutivas() {
    Eventos eventos;
    MonitorRW monitor(Politica::Fifo, &eventos);
    Puerta puerta;

    id_hebra = 0;
    monitor.entrar_lectura();

    std::thread lectora([&] {
        id_hebra = 1;
        monitor.entrar_lectura();
        puerta.esperar();
        monitor.salir_lectura();
    });

    bool simultaneas = eventos.esperar(1, Accion::Entra);
    auto durante = eventos.copia();
    simultaneas = simultaneas && !contiene(durante, 0, Accion::Sale);

    monitor.salir_lectura();
    puerta.abrir();
    lectora.join();

    auto traza = eventos.copia();

    exigir(simultaneas, "dos lectoras FIFO no compartieron acceso");
    exigir(indice(traza, 1, Accion::Entra) <
           indice(traza, 0, Accion::Sale),
           "lectora consecutiva espero la salida");

    comprobar_traza(monitor, Politica::Fifo, 2);

    exigir(verificar_politica(monitor, Politica::Fifo, true).evidencia == 1,
           "no se registro evidencia de lectoras simultaneas");
}

void prueba_escritora_no_puede_ser_adelantada() {
    Eventos eventos;
    MonitorRW monitor(Politica::Fifo, &eventos);
    Puerta puerta;

    id_hebra = 0;
    monitor.entrar_lectura();

    std::thread escritora([&] {
        id_hebra = 1;
        monitor.entrar_escritura();
        puerta.esperar();
        monitor.salir_escritura();
    });

    bool preparada = eventos.esperar(1, Accion::Pide);

    std::thread lectora([&] {
        id_hebra = 2;
        monitor.entrar_lectura();
        monitor.salir_lectura();
    });

    preparada = eventos.esperar(2, Accion::Pide) && preparada;
    monitor.salir_lectura();

    const bool escritora_entro = eventos.esperar(1, Accion::Entra);
    auto durante = eventos.copia();
    const bool lectora_no_adelanto =
        !contiene(durante, 2, Accion::Entra);

    puerta.abrir();
    escritora.join();
    lectora.join();

    auto traza = eventos.copia();

    exigir(preparada && escritora_entro,
           "no se registro la competencia FIFO");
    exigir(lectora_no_adelanto,
           "lectora posterior entro antes que escritora");
    exigir(indice(traza, 0, Accion::Sale) <
           indice(traza, 1, Accion::Entra),
           "escritora entro con lectora activa");
    exigir(indice(traza, 1, Accion::Sale) <
           indice(traza, 2, Accion::Entra),
           "lectora posterior adelanto escritora");

    comprobar_traza(monitor, Politica::Fifo, 3);
}

void prueba_escritoras_en_orden() {
    Eventos eventos;
    MonitorRW monitor(Politica::Fifo, &eventos);
    Puerta puerta;

    id_hebra = 0;
    monitor.entrar_lectura();

    std::thread primera([&] {
        id_hebra = 1;
        monitor.entrar_escritura();
        puerta.esperar();
        monitor.salir_escritura();
    });

    bool preparada = eventos.esperar(1, Accion::Pide);

    std::thread segunda([&] {
        id_hebra = 2;
        monitor.entrar_escritura();
        monitor.salir_escritura();
    });

    preparada = eventos.esperar(2, Accion::Pide) && preparada;
    monitor.salir_lectura();

    bool entro_primera = eventos.esperar(1, Accion::Entra);
    auto durante = eventos.copia();
    bool segunda_no_adelanto =
        !contiene(durante, 2, Accion::Entra);

    puerta.abrir();
    primera.join();
    segunda.join();

    auto traza = eventos.copia();

    exigir(preparada && entro_primera,
           "no se registro la competencia entre escritoras");
    exigir(segunda_no_adelanto,
           "segunda escritora adelanto a la primera");
    exigir(indice(traza, 1, Accion::Sale) <
           indice(traza, 2, Accion::Entra),
           "dos escritoras se superpusieron");

    comprobar_traza(monitor, Politica::Fifo, 3);
}

void prueba_politicas_y_notraza() {
    for (auto politica :
         {Politica::Lectores, Politica::Escritores, Politica::Fifo}) {

        MonitorRW monitor(politica);

        id_hebra = 0;
        monitor.entrar_lectura();
        monitor.salir_lectura();

        id_hebra = 1;
        monitor.entrar_escritura();
        monitor.salir_escritura();

        comprobar_traza(monitor, politica, 2);

        exigir(
            verificar_politica(monitor, politica, true).estado ==
                EstadoPolitica::Correcta,
            "la politica no fue validada"
        );

        MonitorRW sin_traza(politica, nullptr, false);

        sin_traza.entrar_lectura();
        sin_traza.salir_lectura();

        exigir(sin_traza.obtener_traza().empty(),
               "notraza guardo eventos");

        exigir(
            verificar_politica(sin_traza, politica, false).estado ==
                EstadoPolitica::Desactivada,
            "estado notraza incorrecto"
        );
    }
}

void prueba_trazas_alteradas() {
    const std::vector<AnotacionTraza> original = {
        {0, Tipo::Lectora, Accion::Pide, 0},
        {0, Tipo::Lectora, Accion::Entra, 0},
        {1, Tipo::Lectora, Accion::Pide, 1},
        {1, Tipo::Lectora, Accion::Entra, 1},
        {0, Tipo::Lectora, Accion::Sale, 0},
        {1, Tipo::Lectora, Accion::Sale, 1}
    };

    auto resultado = verificar_traza(original, Politica::Fifo);

    exigir(resultado.valida && resultado.evidencia == 1,
           "traza FIFO valida rechazada");

    auto alterada = original;
    alterada[3].turno = 9;

    resultado = verificar_traza(alterada, Politica::Fifo);

    exigir(!resultado.valida && resultado.indice == 3,
           "alteracion del turno no detectada");

    alterada = original;
    alterada[2].turno = 9;

    exigir(!verificar_traza(alterada, Politica::Fifo).valida,
           "alteracion de Pide no detectada");

    alterada = original;
    alterada.pop_back();

    exigir(!verificar_traza(alterada, Politica::Fifo).valida,
           "traza incompleta aceptada");
}

int main() {
    try {
        prueba_lectoras_consecutivas();
        prueba_escritora_no_puede_ser_adelantada();
        prueba_escritoras_en_orden();
        prueba_politicas_y_notraza();
        prueba_trazas_alteradas();

        std::cout << "Pruebas FIFO y trazas correctas\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fallo FIFO/traza: " << e.what() << '\n';
        return 1;
    }
}
