#include "metricas.hpp"
#include <algorithm>
#include <cerrno>
#include <ctime>
#include <limits>
#include <system_error>

namespace {
void sumar_saturado(std::uint64_t& destino, std::uint64_t valor, bool& saturado) noexcept {
    if (valor > std::numeric_limits<std::uint64_t>::max() - destino) {
        destino = std::numeric_limits<std::uint64_t>::max();
        saturado = true;
    } else destino += valor;
}
}
void incrementar_fallo(std::uint64_t& contador, bool& saturado) noexcept {
    sumar_saturado(contador, 1, saturado);
}
void acumular_fallos(FallosAdmision& d, const FallosAdmision& o) noexcept {
    d.saturado = d.saturado || o.saturado;
    sumar_saturado(d.recurso_ocupado, o.recurso_ocupado, d.saturado);
    sumar_saturado(d.escritoras_esperando, o.escritoras_esperando, d.saturado);
    sumar_saturado(d.cas_fallido, o.cas_fallido, d.saturado);
}
void Esperas::agregar(std::uint64_t ns) noexcept {
    ++solicitudes;
    suma_ns += ns;
    max_ns = std::max(max_ns, ns);
}
void Esperas::acumular(const Esperas& o) noexcept {
    solicitudes += o.solicitudes;
    suma_ns += o.suma_ns;
    max_ns = std::max(max_ns, o.max_ns);
}
long double Esperas::media_us() const noexcept {
    return solicitudes == 0 ? 0 : suma_ns / solicitudes / 1000;
}

void Diagnostico::entrar(Tipo tipo) noexcept {
    if (tipo == Tipo::Lectora) {
        const long actuales = lectoras_.fetch_add(1) + 1;
        if (escritoras_.load() != 0) violaciones_.fetch_add(1);
        long maximo = max_lectoras_.load();
        while (actuales > maximo && !max_lectoras_.compare_exchange_weak(maximo, actuales)) {}
    } else {
        const long anteriores = escritoras_.fetch_add(1);
        if (anteriores != 0 || lectoras_.load() != 0) violaciones_.fetch_add(1);
    }
}
void Diagnostico::salir(Tipo tipo) noexcept {
    if (tipo == Tipo::Lectora) lectoras_.fetch_sub(1);
    else escritoras_.fetch_sub(1);
}
long Diagnostico::max_lectoras() const noexcept { return max_lectoras_.load(); }
long Diagnostico::violaciones() const noexcept { return violaciones_.load(); }
bool Diagnostico::vacio() const noexcept { return lectoras_.load() == 0 && escritoras_.load() == 0; }

double cpu_proceso_segundos() {
    timespec t{};
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t) != 0) {
        throw std::system_error(errno, std::generic_category(), "CLOCK_PROCESS_CPUTIME_ID");
    }
    return static_cast<double>(t.tv_sec) + static_cast<double>(t.tv_nsec) / 1e9;
}
const char* fuente_cpu() noexcept {
    return "CLOCK_PROCESS_CPUTIME_ID";
}
