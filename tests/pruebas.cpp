#include "configuracion.hpp"
#include "ejecucion.hpp"
#include "integraciones.hpp"
#include "intervalo_admitido.hpp"
#include "monitor_rw.hpp"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace {
void exigir(bool condicion, const char* mensaje) {
    if (!condicion) throw std::runtime_error(mensaje);
}
template<class F> void rechaza(F funcion) {
    bool rechazado = false;
    try { funcion(); } catch (const std::invalid_argument&) { rechazado = true; }
    exigir(rechazado, "se acepto una entrada invalida");
}
Configuracion argumentos(std::initializer_list<std::string_view> a) {
    return analizar_argumentos(std::vector<std::string_view>(a));
}

// Observador exclusivo de pruebas: no es la traza ni el verificador R6 de P2.
// Ninguna espera retiene este mutex ni llama al monitor: no hay ciclo de locks.
class Eventos : public ObservadorAdmision {
public:
    Eventos() { eventos_.reserve(128); }
    void registrar(const AnotacionTraza& evento) noexcept override {
        std::lock_guard<std::mutex> lock(m_);
        try { eventos_.push_back(evento); } catch (...) { perdidos_ = true; }
        cv_.notify_all();
    }
    bool esperar(int id, Accion accion) {
        std::unique_lock<std::mutex> lock(m_);
        return cv_.wait_for(lock, std::chrono::seconds(5), [&] { return contiene(id, accion); });
    }
    bool esperar_escritora() {
        std::unique_lock<std::mutex> lock(m_);
        return cv_.wait_for(lock, std::chrono::seconds(5), [&] {
            return contiene(1, Accion::Entra) || contiene(3, Accion::Entra);
        });
    }
    std::vector<AnotacionTraza> copia() {
        std::lock_guard<std::mutex> lock(m_);
        exigir(!perdidos_, "se perdieron eventos de prueba");
        return eventos_;
    }
private:
    bool contiene(int id, Accion accion) const {
        return std::any_of(eventos_.begin(), eventos_.end(), [&](const auto& e) {
            return e.hebra == id && e.accion == accion;
        });
    }
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<AnotacionTraza> eventos_;
    bool perdidos_ = false;
};

class PuertaTrabajo {
public:
    void pasar() {
        std::unique_lock<std::mutex> lock(m_);
        ++llegadas_;
        cv_.notify_all();
        cv_.wait(lock, [&] { return abierta_; });
    }
    bool esperar_llegadas(int cantidad) {
        std::unique_lock<std::mutex> lock(m_);
        return cv_.wait_for(lock, std::chrono::seconds(5), [&] { return llegadas_ >= cantidad; });
    }
    void abrir() {
        std::lock_guard<std::mutex> lock(m_);
        abierta_ = true;
        cv_.notify_all();
    }
private:
    std::mutex m_;
    std::condition_variable cv_;
    int llegadas_ = 0;
    bool abierta_ = false;
};

std::size_t indice(const std::vector<AnotacionTraza>& eventos, int id, Accion accion) {
    for (std::size_t i = 0; i < eventos.size(); ++i) {
        if (eventos[i].hebra == id && eventos[i].accion == accion) return i;
    }
    throw std::runtime_error("evento esperado ausente");
}

void prueba_argumentos() {
    const auto c = argumentos({"cond", "lectores", "6", "2", "100", "200", "500", "notraza"});
    exigir(c.lectores == 6 && c.escritores == 2 && !c.traza, "parser valido");
    exigir(argumentos({"atomic", "escritores", "1", "1", "1", "0", "0"}).modo == Modo::Atomic, "modo atomic");
    exigir(argumentos({"cond", "fifo", "1", "1", "1", "0", "0"}).politica == Politica::Fifo, "politica fifo");
    rechaza([] { argumentos({}); });
    rechaza([] { argumentos({"otro", "lectores", "1", "1", "1", "0", "0"}); });
    rechaza([] { argumentos({"cond", "otra", "1", "1", "1", "0", "0"}); });
    for (auto valor : {"-1", "+1", " 1", "1x", "", "999999999999999999999999999999"}) {
        rechaza([&] { argumentos({"cond", "lectores", valor, "1", "1", "0", "0"}); });
    }
    rechaza([] { argumentos({"atomic", "lectores", "1", "1", "1", "0", "0"}); });
    rechaza([] { argumentos({"atomic", "fifo", "1", "1", "1", "0", "0"}); });
    rechaza([] { argumentos({"cond", "lectores", "0", "1", "1", "0", "0"}); });
    rechaza([] { argumentos({"cond", "lectores", "1", "0", "1", "0", "0"}); });
    rechaza([] { argumentos({"cond", "lectores", "999", "2", "1", "0", "0"}); });
    rechaza([] { argumentos({"cond", "lectores", "1", "1", "0", "0", "0"}); });
    rechaza([] { argumentos({"cond", "lectores", "1", "1", "1", "-1", "0"}); });
    rechaza([] { argumentos({"cond", "lectores", "1", "1", "1", "0", "-1"}); });
    rechaza([] { argumentos({"cond", "lectores", "1", "1", "1", "0", "0", "otra"}); });
}

void prueba_limites() {
    const long limite = std::numeric_limits<long>::max();
    Configuracion c;
    c.operaciones = limite / 2;
    validar_configuracion(c); // no ejecutar: solo frontera aritmética
    ++c.operaciones;
    rechaza([&] { validar_configuracion(c); });
    c.lectores = 999;
    c.operaciones = limite / 1000;
    c.dur_lec_us = limite;
    validar_configuracion(c);
    ++c.operaciones;
    rechaza([&] { validar_configuracion(c); });
    c.lectores = limite;
    rechaza([&] { validar_configuracion(c); });
    c = {};
    c.modo = static_cast<Modo>(999);
    rechaza([&] { validar_configuracion(c); });
    std::cout << "  sizeof(long)=" << sizeof(long) << " LONG_MAX=" << limite << '\n';
}

void prueba_registro_diagnostico() {
    Registro r;
    exigir(registro_consistente(r), "registro inicial");
    escribir_registro(r);
    exigir(r.version == 1 && r.doble_version == 2 && registro_consistente(r), "registro escrito");
    r.doble_version = 3;
    exigir(!registro_consistente(r), "inconsistencia no detectada");
    r.version = std::numeric_limits<long>::max();
    exigir(!registro_consistente(r), "comprobacion desborda con un registro corrupto");
    Diagnostico d;
    d.entrar(Tipo::Escritora);
    d.entrar(Tipo::Escritora);
    d.entrar(Tipo::Lectora);
    exigir(d.violaciones() == 2, "diagnostico no detecta solapamientos inyectados");
    d.salir(Tipo::Lectora);
    d.salir(Tipo::Escritora);
    d.salir(Tipo::Escritora);
    exigir(d.vacio(), "contadores desequilibrados");
}

void prueba_prioridad_lectores() {
    Eventos eventos;
    MonitorRW monitor(Politica::Lectores, &eventos);
    Diagnostico d;
    PuertaTrabajo puerta;
    id_hebra = 0;
    monitor.entrar_escritura();
    d.entrar(Tipo::Escritora);
    std::thread escritora([&] {
        id_hebra = 1;
        monitor.entrar_escritura();
        d.entrar(Tipo::Escritora);
        d.salir(Tipo::Escritora);
        monitor.salir_escritura();
    });
    bool preparada = eventos.esperar(1, Accion::Pide);
    auto leer = [&](int id) {
        id_hebra = id;
        monitor.entrar_lectura();
        d.entrar(Tipo::Lectora);
        puerta.pasar(); // barrera de prueba DENTRO del intervalo, fuera del monitor
        d.salir(Tipo::Lectora);
        monitor.salir_lectura();
    };
    std::thread lectora2(leer, 2);
    std::thread lectora3(leer, 3);
    preparada = eventos.esperar(2, Accion::Pide) && preparada;
    preparada = eventos.esperar(3, Accion::Pide) && preparada;
    d.salir(Tipo::Escritora);
    monitor.salir_escritura();
    bool concurrentes = puerta.esperar_llegadas(2);
    // También una lectora NUEVA adelanta a la escritora pendiente.
    std::thread lectora4(leer, 4);
    concurrentes = puerta.esperar_llegadas(3) && concurrentes;
    puerta.abrir();
    lectora2.join(); lectora3.join(); lectora4.join(); escritora.join();
    const auto e = eventos.copia();
    exigir(preparada && concurrentes, "lectoras pendientes o nuevas no tienen prioridad/concurrencia");
    for (int id : {2, 3, 4}) {
        exigir(indice(e, id, Accion::Sale) < indice(e, 1, Accion::Entra), "escritora adelanto lectoras");
    }
    exigir(indice(e, 4, Accion::Pide) + 1 == indice(e, 4, Accion::Entra), "lectora habilitada no entra inmediatamente");
    exigir(d.max_lectoras() == 3 && d.violaciones() == 0 && d.vacio(), "concurrencia/exclusion lectores");
}

void prueba_prioridad_escritores() {
    Eventos eventos;
    MonitorRW monitor(Politica::Escritores, &eventos);
    Diagnostico d;
    PuertaTrabajo puerta;
    id_hebra = 0;
    monitor.entrar_lectura();
    d.entrar(Tipo::Lectora);
    auto escribir = [&](int id) {
        id_hebra = id;
        monitor.entrar_escritura();
        d.entrar(Tipo::Escritora);
        puerta.pasar();
        d.salir(Tipo::Escritora);
        monitor.salir_escritura();
    };
    std::thread escritora1(escribir, 1);
    bool preparada = eventos.esperar(1, Accion::Pide);
    std::thread lectora([&] {
        id_hebra = 2;
        monitor.entrar_lectura();
        d.entrar(Tipo::Lectora);
        d.salir(Tipo::Lectora);
        monitor.salir_lectura();
    });
    preparada = eventos.esperar(2, Accion::Pide) && preparada;
    // Esta escritora pide después de la lectora, pero también la antecede.
    std::thread escritora3(escribir, 3);
    preparada = eventos.esperar(3, Accion::Pide) && preparada;
    d.salir(Tipo::Lectora);
    monitor.salir_lectura();
    preparada = eventos.esperar_escritora() && preparada;
    puerta.abrir();
    escritora1.join(); escritora3.join(); lectora.join();
    const auto e = eventos.copia();
    exigir(preparada, "no se logro el entrelazamiento de escritores");
    exigir(indice(e, 2, Accion::Pide) < indice(e, 3, Accion::Pide), "orden de solicitudes de prueba");
    for (int id : {1, 3}) {
        exigir(indice(e, id, Accion::Sale) < indice(e, 2, Accion::Entra), "lectora adelanto escritora pendiente");
    }
    exigir(d.violaciones() == 0 && d.vacio(), "exclusion escritores");
}

void prueba_lectoras_concurrentes() {
    for (auto politica : {Politica::Lectores, Politica::Escritores}) {
        MonitorRW monitor(politica);
        Diagnostico d;
        PuertaTrabajo puerta;
        monitor.entrar_lectura();
        d.entrar(Tipo::Lectora);
        std::thread lectora([&] {
            monitor.entrar_lectura();
            d.entrar(Tipo::Lectora);
            puerta.pasar();
            d.salir(Tipo::Lectora);
            monitor.salir_lectura();
        });
        const bool entro = puerta.esperar_llegadas(1);
        d.salir(Tipo::Lectora);
        monitor.salir_lectura();
        puerta.abrir();
        lectora.join();
        exigir(entro && d.max_lectoras() == 2 && d.violaciones() == 0 && d.vacio(), "lectoras serializadas");
    }
}

void prueba_carga_compartida() {
    for (auto politica : {Politica::Lectores, Politica::Escritores}) {
        for (long pausa : {0L, 50L}) {
            Configuracion c;
            c.politica = politica;
            c.lectores = 4;
            c.escritores = 3;
            c.operaciones = 100;
            c.dur_lec_us = c.dur_esc_us = pausa;
            auto control = crear_control(c);
            const auto r = ejecutar(c, *control);
            exigir(r.correcto(c), "carga: invariantes o valores finales incorrectos");
            exigir(r.espera_lectoras.solicitudes == 400 && r.espera_escritoras.solicitudes == 300, "esperas por solicitud");
            exigir(r.pared_s > 0 && r.cpu_s >= 0, "relojes invalidos");
            exigir(r.espera_lectoras.media_us() <= r.espera_lectoras.max_ns / 1000.0L, "media lectura supera maximo");
            exigir(r.espera_escritoras.media_us() <= r.espera_escritoras.max_ns / 1000.0L, "media escritura supera maximo");
            std::cout << "  carga " << nombre(politica) << " pausa_us=" << pausa
                      << " version=" << r.registro.version << " max_lectoras=" << r.max_lectoras
                      << " pared_s=" << r.pared_s << " cpu_s=" << r.cpu_s << '\n';
        }
    }
}

void prueba_fallo_creacion() {
    for (int fallar_en : {0, 3, 5}) {
        Configuracion c;
        c.politica = Politica::Escritores;
        c.lectores = 4;
        c.escritores = 2;
        c.operaciones = 10;
        MonitorRW monitor(c.politica);
        int intentos = 0;
        std::atomic<int> terminadas{0};
        const auto r = ejecutar(c, monitor, [&](std::function<void()> tarea) -> std::thread {
            if (intentos++ == fallar_en) {
                throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
            }
            return std::thread([&, tarea = std::move(tarea)] { tarea(); ++terminadas; });
        });
        exigir(r.hebras_creadas == fallar_en && terminadas == fallar_en, "no espero las hebras ya creadas");
        exigir(!r.correcto(c) && r.errores.size() == 1, "fallo de creacion no informado");
        exigir(r.diagnostico_vacio && r.violaciones_exclusion == 0, "ejecucion parcial desequilibrada");
        exigir(r.registro.version == std::max(0, fallar_en - 4) * 10, "escrituras de hebras ya creadas");
    }
}

void prueba_error_hebra() {
    class ControlConFallo : public ControlRW {
        MonitorRW monitor_{Politica::Escritores};
    public:
        void entrar_lectura() override {
            if (id_hebra == 1) throw std::runtime_error("fallo inyectado antes de admitir");
            monitor_.entrar_lectura();
        }
        void salir_lectura() override { monitor_.salir_lectura(); }
        void entrar_escritura() override { monitor_.entrar_escritura(); }
        void salir_escritura() override { monitor_.salir_escritura(); }
    } control;
    Configuracion c;
    c.lectores = 2;
    c.politica = Politica::Escritores;
    c.operaciones = 5;
    const auto r = ejecutar(c, control);
    exigir(!r.correcto(c) && r.hebras_creadas == 3 && r.errores.size() == 1, "error de hebra perdido");
    exigir(r.lecturas == 5 && r.escrituras == 5 && r.diagnostico_vacio, "otras hebras no terminaron");
}

void prueba_error_dentro_del_intervalo() {
    for (auto politica : {Politica::Lectores, Politica::Escritores}) {
        for (auto tipo : {Tipo::Lectora, Tipo::Escritora}) {
            Eventos eventos;
            MonitorRW monitor(politica, &eventos);
            Diagnostico diagnostico;
            Registro registro;
            PuertaTrabajo causar_fallo;
            std::exception_ptr error_simulado;
            std::exception_ptr error_escritora;
            std::exception_ptr error_lectora;
            bool lectura_consistente = false;
            // Misma clase RAII utilizada por ejecutar(). La excepción ocurre
            // después del acceso al registro, mientras la admisión sigue activa.
            std::thread fallida([&] {
                id_hebra = 0;
                try {
                    if (tipo == Tipo::Lectora) monitor.entrar_lectura();
                    else monitor.entrar_escritura();
                    IntervaloAdmitido intervalo(monitor, diagnostico, tipo);
                    if (tipo == Tipo::Escritora) escribir_registro(registro);
                    else exigir(registro_consistente(registro), "lectura inicial inconsistente");
                    causar_fallo.pasar();
                    throw std::runtime_error("fallo de trabajo dentro de la admision");
                } catch (...) { error_simulado = std::current_exception(); }
            });
            bool preparada = causar_fallo.esperar_llegadas(1);
            std::thread escritora([&] {
                id_hebra = 1;
                try {
                    monitor.entrar_escritura();
                    IntervaloAdmitido intervalo(monitor, diagnostico, Tipo::Escritora);
                    escribir_registro(registro);
                } catch (...) { error_escritora = std::current_exception(); }
            });
            preparada = eventos.esperar(1, Accion::Pide) && preparada;
            std::thread lectora([&] {
                id_hebra = 2;
                try {
                    monitor.entrar_lectura();
                    IntervaloAdmitido intervalo(monitor, diagnostico, Tipo::Lectora);
                    lectura_consistente = registro_consistente(registro);
                } catch (...) { error_lectora = std::current_exception(); }
            });
            preparada = eventos.esperar(2, Accion::Pide) && preparada;
            causar_fallo.abrir();
            fallida.join(); escritora.join(); lectora.join();

            exigir(preparada && error_simulado && !error_escritora && !error_lectora,
                   "no se recupero el fallo de trabajo");
            try { std::rethrow_exception(error_simulado); }
            catch (const std::runtime_error& e) {
                exigir(std::string(e.what()) == "fallo de trabajo dentro de la admision",
                       "la excepcion no ocurrio en el punto previsto");
            }
            const auto e = eventos.copia();
            exigir(indice(e, 0, Accion::Sale) < indice(e, 1, Accion::Entra),
                   "la escritora entro antes de liberar la admision fallida");
            exigir(lectura_consistente && registro_consistente(registro), "registro tras fallo");
            const long esperada = tipo == Tipo::Escritora ? 2 : 1;
            exigir(registro.version == esperada && registro.doble_version == 2 * esperada,
                   "el fallo perdio escrituras completas");
            exigir(diagnostico.vacio() && diagnostico.violaciones() == 0,
                   "el fallo dejo una admision o diagnostico activo");
            std::cout << "  recuperacion " << nombre(politica)
                      << " tipo=" << (tipo == Tipo::Lectora ? "lectora" : "escritora")
                      << " version=" << registro.version << " diagnostico_vacio=SI\n";
        }
    }
}

void prueba_contencion_sin_pausas() {
    struct Caso { long lectoras; long escritoras; long operaciones; };
    const Caso casos[] = {{12, 2, 2000}, {2, 12, 2000}, {8, 8, 10000}, {24, 24, 500}};
    for (auto politica : {Politica::Lectores, Politica::Escritores}) {
        for (const auto& caso : casos) {
            for (int repeticion = 1; repeticion <= 2; ++repeticion) {
                Configuracion c;
                c.politica = politica;
                c.lectores = caso.lectoras;
                c.escritores = caso.escritoras;
                c.operaciones = caso.operaciones;
                c.traza = false;
                MonitorRW monitor(politica);
                PuertaTrabajo inicio;
                long creadas = 0;
                // Barrera exclusiva del arnés, antes de las tareas. Si una
                // creación falla, abrirla permite reunir las hebras existentes.
                const auto r = ejecutar(c, monitor, [&](std::function<void()> tarea) -> std::thread {
                    try {
                        std::thread hebra([&, tarea = std::move(tarea)] {
                            inicio.pasar();
                            tarea();
                        });
                        if (++creadas == c.lectores + c.escritores) inicio.abrir();
                        return hebra;
                    } catch (...) {
                        inicio.abrir();
                        throw;
                    }
                });
                exigir(r.correcto(c), "contencion: registro, exclusion o carga incorrectos");
                exigir(r.espera_lectoras.solicitudes == c.lectores * c.operaciones &&
                       r.espera_escritoras.solicitudes == c.escritores * c.operaciones,
                       "contencion: se perdieron solicitudes");
                exigir(r.max_lectoras >= 1 && r.max_lectoras <= c.lectores,
                       "contencion: maximo de lectoras fuera de rango");
                std::cout << "  contencion " << nombre(politica)
                          << " L=" << c.lectores << " E=" << c.escritores
                          << " K=" << c.operaciones << " rep=" << repeticion
                          << " operaciones=" << r.lecturas + r.escrituras
                          << " version=" << r.registro.version
                          << " max_lectoras=" << r.max_lectoras
                          << " inconsistencias=" << r.inconsistencias
                          << " violaciones=" << r.violaciones_exclusion << '\n';
            }
        }
    }
}

void prueba_integracion_pendiente() {
    Configuracion c;
    c.politica = Politica::Fifo;
    rechaza([&] { crear_control(c); });
    c.modo = Modo::Atomic;
    c.politica = Politica::Escritores;
    rechaza([&] { crear_control(c); });
    MonitorRW monitor(Politica::Lectores);
    exigir(verificar_politica(monitor, Politica::Lectores, true).estado == EstadoPolitica::Pendiente, "verificador ficticio");
    exigir(verificar_politica(monitor, Politica::Lectores, false).estado == EstadoPolitica::Desactivada, "notraza");
}
}

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> pruebas = {
        {"argumentos", prueba_argumentos}, {"limites long", prueba_limites},
        {"registro y diagnostico", prueba_registro_diagnostico},
        {"prioridad lectores: pendientes y nuevas", prueba_prioridad_lectores},
        {"prioridad escritores: dos pendientes", prueba_prioridad_escritores},
        {"lectoras concurrentes en ambas politicas", prueba_lectoras_concurrentes},
        {"carga compartida y metricas", prueba_carga_compartida},
        {"fallo de creacion y join", prueba_fallo_creacion},
        {"error dentro de una hebra", prueba_error_hebra},
        {"excepcion durante lectura y escritura admitidas", prueba_error_dentro_del_intervalo},
        {"contencion sin pausas en ambas politicas", prueba_contencion_sin_pausas},
        {"integraciones pendientes explicitas", prueba_integracion_pendiente}
    };
    int fallos = 0;
    for (const auto& prueba : pruebas) {
        try { prueba.second(); std::cout << "OK " << prueba.first << std::endl; }
        catch (const std::exception& e) {
            ++fallos;
            std::cerr << "FALLO " << prueba.first << ": " << e.what() << std::endl;
        }
    }
    std::cout << "pruebas=" << pruebas.size() << " fallos=" << fallos << std::endl;
    return fallos == 0 ? 0 : 1;
}
