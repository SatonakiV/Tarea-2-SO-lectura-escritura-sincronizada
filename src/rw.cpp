#include "ejecucion.hpp"
#include "integraciones.hpp"
#include <exception>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

int main(int argc, char* argv[]) {
    try {
        std::vector<std::string_view> argumentos;
        for (int i = 1; i < argc; ++i) argumentos.emplace_back(argv[i]);
        const auto c = analizar_argumentos(argumentos);
        auto control = crear_control(c);
        const auto r = ejecutar(c, *control);
        const auto politica = c.modo == Modo::Cond
            ? verificar_politica(*control, c.politica, c.traza)
            : ResultadoPolitica{EstadoPolitica::Desactivada, 0};
        const bool correcto = r.correcto(c) && politica.estado != EstadoPolitica::Incorrecta;
        std::cout << std::fixed << std::setprecision(6)
            << "modo=" << nombre(c.modo) << " politica=" << nombre(c.politica)
            << " L=" << c.lectores << " E=" << c.escritores << " K=" << c.operaciones
            << " dur_lec_us=" << c.dur_lec_us << " dur_esc_us=" << c.dur_esc_us << '\n'
            << "hebras_creadas=" << r.hebras_creadas
            << " operaciones_previstas=" << (c.lectores + c.escritores) * c.operaciones
            << " operaciones_realizadas=" << r.lecturas + r.escrituras
            << " lecturas=" << r.lecturas << " escrituras=" << r.escrituras << '\n'
            << "version_final=" << r.registro.version << " doble_version_final=" << r.registro.doble_version
            << " inconsistencias=" << r.inconsistencias
            << " violaciones_exclusion=" << r.violaciones_exclusion
            << " diagnostico_vacio=" << (r.diagnostico_vacio ? "SI" : "NO")
            << " max_lectoras=" << r.max_lectoras << '\n'
            << "pared_s=" << r.pared_s << " cpu_s=" << r.cpu_s << " fuente_cpu=" << fuente_cpu() << '\n'
            << "solicitudes_lectura=" << r.espera_lectoras.solicitudes
            << " espera_lectura_media_us=" << r.espera_lectoras.media_us()
            << " espera_lectura_max_us=" << r.espera_lectoras.max_ns / 1000.0L << '\n'
            << "solicitudes_escritura=" << r.espera_escritoras.solicitudes
            << " espera_escritura_media_us=" << r.espera_escritoras.media_us()
            << " espera_escritura_max_us=" << r.espera_escritoras.max_ns / 1000.0L << '\n'
            << "fallos_aplican=" << (c.modo == Modo::Atomic ? "SI" : "NO")
            << " fallos_recurso_ocupado=" << r.fallos.recurso_ocupado
            << " fallos_escritoras_esperando=" << r.fallos.escritoras_esperando
            << " fallos_cas=" << r.fallos.cas_fallido
            << " fallos_saturados=" << (r.fallos.saturado ? "SI" : "NO") << '\n'
            << "r2_ok=" << (r.correcto(c) ? "SI" : "NO")
            << " correcto=" << (correcto ? "SI" : "NO");
        switch (politica.estado) {
        case EstadoPolitica::Correcta: std::cout << " traza=activa politica_ok=SI evidencia=" << politica.evidencia; break;
        case EstadoPolitica::Incorrecta: std::cout << " traza=activa politica_ok=NO evidencia=" << politica.evidencia; break;
        case EstadoPolitica::Pendiente: std::cout << " traza=pendiente politica_ok=PENDIENTE evidencia=NA"; break;
        case EstadoPolitica::Desactivada: std::cout << " traza=desactivada politica_ok=NA evidencia=NA"; break;
        }
        std::cout << '\n';
        for (const auto& error : r.errores) std::cerr << "Error: " << error << '\n';
        return correcto ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n' << "Uso: " << uso() << '\n';
        return 1;
    }
}
