#include "ejecucion.hpp"
#include "intervalo_admitido.hpp"
#include <algorithm>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <utility>

namespace {
using Reloj = std::chrono::steady_clock;
struct ResultadoHebra {
    long completadas = 0;
    long inconsistencias = 0;
    Esperas esperas;
    FallosAdmision fallos;
    std::exception_ptr error;
};

class ReunirHebras {
public:
    explicit ReunirHebras(std::vector<std::thread>& hebras) : hebras_(hebras) {}
    ~ReunirHebras() { reunir(); }
    void reunir() {
        for (auto& hebra : hebras_) if (hebra.joinable()) hebra.join();
    }
private:
    std::vector<std::thread>& hebras_;
};

void simular_trabajo(long microsegundos) {
    // Acotar cada sleep evita desbordamientos internos al convertir un long
    // grande a nanosegundos o sumarlo al reloj. Cero no llama a sleep_for.
    while (microsegundos > 0) {
        const long tramo = std::min(microsegundos, 1000000L);
        std::this_thread::sleep_for(std::chrono::microseconds(tramo));
        microsegundos -= tramo;
    }
}

std::string describir(std::exception_ptr error) {
    try { std::rethrow_exception(error); }
    catch (const std::exception& e) { return e.what(); }
    catch (...) { return "excepcion no estandar"; }
}
}

bool ResultadoEjecucion::correcto(const Configuracion& c) const noexcept {
    return errores.empty() && hebras_creadas == c.lectores + c.escritores &&
        lecturas == c.lectores * c.operaciones && escrituras == c.escritores * c.operaciones &&
        registro.version == c.escritores * c.operaciones &&
        registro.doble_version == 2 * (c.escritores * c.operaciones) &&
        inconsistencias == 0 && violaciones_exclusion == 0 && diagnostico_vacio;
}

ResultadoEjecucion ejecutar(const Configuracion& c, ControlRW& control, LanzadorHebras lanzador) {
    validar_configuracion(c);
    Registro registro;
    Diagnostico diagnostico;
    const long cantidad = c.lectores + c.escritores;
    std::vector<ResultadoHebra> resultados(static_cast<std::size_t>(cantidad));
    std::vector<std::thread> hebras;
    hebras.reserve(static_cast<std::size_t>(cantidad));
    ReunirHebras reunir(hebras); // también protege salidas excepcionales
    ResultadoEjecucion resultado;
    std::exception_ptr error_creacion;

    const auto inicio_pared = Reloj::now();
    const double inicio_cpu = cpu_proceso_segundos();
    for (long i = 0; i < cantidad; ++i) {
        try {
            auto tarea = [&, i] {
                id_hebra = static_cast<int>(i); // 0..L-1 lectoras; L..L+E-1 escritoras
                fallos_hebra = {};
                auto& local = resultados[static_cast<std::size_t>(i)];
                const Tipo tipo = i < c.lectores ? Tipo::Lectora : Tipo::Escritora;
                try {
                    for (long k = 0; k < c.operaciones; ++k) {
                        const auto pide = Reloj::now();
                        if (tipo == Tipo::Lectora) control.entrar_lectura();
                        else control.entrar_escritura();
                        const auto entra = Reloj::now();
                        IntervaloAdmitido intervalo(control, diagnostico, tipo);
                        local.esperas.agregar(static_cast<std::uint64_t>(
                            std::chrono::duration_cast<std::chrono::nanoseconds>(entra - pide).count()));
                        if (tipo == Tipo::Lectora) {
                            if (!registro_consistente(registro)) ++local.inconsistencias;
                            simular_trabajo(c.dur_lec_us);
                        } else {
                            escribir_registro(registro);
                            simular_trabajo(c.dur_esc_us);
                        }
                        ++local.completadas;
                    }
                } catch (...) { local.error = std::current_exception(); }
                local.fallos = fallos_hebra;
            };
            std::thread hebra = lanzador ? lanzador(std::move(tarea)) : std::thread(std::move(tarea));
            if (!hebra.joinable()) throw std::runtime_error("el lanzador devolvio una hebra no valida");
            hebras.push_back(std::move(hebra)); // capacidad reservada, movimiento noexcept
        } catch (...) {
            error_creacion = std::current_exception();
            break;
        }
    }
    reunir.reunir(); // no informar ni retornar antes de completar estos join()
    const double fin_cpu = cpu_proceso_segundos();
    const auto fin_pared = Reloj::now();
    resultado.pared_s = std::chrono::duration<double>(fin_pared - inicio_pared).count();
    resultado.cpu_s = fin_cpu - inicio_cpu;
    resultado.hebras_creadas = static_cast<long>(hebras.size());
    resultado.registro = registro;
    resultado.violaciones_exclusion = diagnostico.violaciones();
    resultado.max_lectoras = diagnostico.max_lectoras();
    resultado.diagnostico_vacio = diagnostico.vacio();
    if (error_creacion) resultado.errores.push_back("creacion de hebra: " + describir(error_creacion));
    for (long i = 0; i < resultado.hebras_creadas; ++i) {
        const auto& local = resultados[static_cast<std::size_t>(i)];
        if (i < c.lectores) {
            resultado.lecturas += local.completadas;
            resultado.espera_lectoras.acumular(local.esperas);
        } else {
            resultado.escrituras += local.completadas;
            resultado.espera_escritoras.acumular(local.esperas);
        }
        resultado.inconsistencias += local.inconsistencias;
        acumular_fallos(resultado.fallos, local.fallos);
        if (local.error) resultado.errores.push_back("hebra " + std::to_string(i) + ": " + describir(local.error));
    }
    return resultado;
}
