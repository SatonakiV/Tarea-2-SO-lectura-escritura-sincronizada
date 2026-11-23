#include "verificador.hpp"
#include <stdexcept>

bool aplicar_evento(EstadoTraza& estado, const AnotacionTraza& evento, std::string& error)
{
    error.clear();

    auto it = estado.solicitudes.find(evento.hebra);

    // Errores relacionados con el turno de la hebra lectora
    if (evento.tipo == Tipo::Lectora) {

        // Procesamiento de pedir entrada y quedar en conteo de en cola
        if (evento.accion == Accion::Pide) {

            if (it != estado.solicitudes.end()) {
                error = "La hebra ya tiene una solicitud sin terminar";
                return false;
            }

            estado.solicitudes.emplace(
                evento.hebra,
                Solicitud{evento.tipo, FaseSolicitud::Pendiente, evento.turno}
            );

            ++estado.lectoras_pendientes;
        }
        // Procesamiento de la Entrada
        else if (evento.accion == Accion::Entra) {

            if (it == estado.solicitudes.end()) {
                error = "La hebra no tiene una solicitud previa";
                return false;
            }

            if (it->second.tipo != evento.tipo ||
                it->second.turno != evento.turno) {
                error = "El tipo o turno no coincide con la solicitud";
                return false;
            }

            if (it->second.fase != FaseSolicitud::Pendiente) {
                error = "La hebra intenta entrar sin estar pendiente";
                return false;
            }

            if (estado.lectoras_pendientes == 0) {
                error = "Una lectora entra sin una solicitud pendiente";
                return false;
            }

            if (estado.escritoras_activas != 0) {
                error = "Una lectora entra mientras hay una escritora activa";
                return false;
            }

            --estado.lectoras_pendientes;
            ++estado.lectoras_activas;

            it->second.fase = FaseSolicitud::Activa;
        }

        // Procesamiento de la Salida
        else if (evento.accion == Accion::Sale) {

            if (it == estado.solicitudes.end()) {
                error = "La hebra no tiene una solicitud previa";
                return false;
            }

            if (it->second.tipo != evento.tipo ||
                it->second.turno != evento.turno) {
                error = "El tipo o turno no coincide con la solicitud";
                return false;
            }

            if (it->second.fase != FaseSolicitud::Activa) {
                error = "La hebra sale sin haber entrado";
                return false;
            }

            if (estado.lectoras_activas == 0) {
                error = "Una lectora sale sin estar activa";
                return false;
            }

            --estado.lectoras_activas;

            estado.solicitudes.erase(it);
        }

        // si no corresponde a ninguna de las acciones conocidas (Pide, Entra, Sale) se reporta error
        else {
            error = "Accion desconocida";
            return false;
        }

    }

    // Errores relacionados con el turno de la hebra Escritora
    else if (evento.tipo == Tipo::Escritora) {

        // Procesamiento de pedir entrada y quedar en conteo de en cola
        if (evento.accion == Accion::Pide) {

            if (it != estado.solicitudes.end()) {
                error = "La hebra ya tiene una solicitud sin terminar";
                return false;
            }

            estado.solicitudes.emplace(
                evento.hebra,
                Solicitud{evento.tipo, FaseSolicitud::Pendiente, evento.turno}
            );

            ++estado.escritoras_pendientes;
        }

        // Procesamiento de la Entrada
        else if (evento.accion == Accion::Entra) {

            if (it == estado.solicitudes.end()) {
                error = "La hebra no tiene una solicitud previa";
                return false;
            }

            if (it->second.tipo != evento.tipo ||
                it->second.turno != evento.turno) {
                error = "El tipo o turno no coincide con la solicitud";
                return false;
            }

            if (it->second.fase != FaseSolicitud::Pendiente) {
                error = "La hebra intenta entrar sin estar pendiente";
                return false;
            }

            if (estado.escritoras_pendientes == 0) {
                error = "Una escritora entra sin una solicitud pendiente";
                return false;
            }

            if (estado.lectoras_activas != 0 ||
                estado.escritoras_activas != 0) {
                error = "Una escritora entra mientras el recurso esta ocupado";
                return false;
            }

            --estado.escritoras_pendientes;
            ++estado.escritoras_activas;

            it->second.fase = FaseSolicitud::Activa;
        }

        // Procesamiento de la Salida
        else if (evento.accion == Accion::Sale) {

            if (it == estado.solicitudes.end()) {
                error = "La hebra no tiene una solicitud previa";
                return false;
            }

            if (it->second.tipo != evento.tipo ||
                it->second.turno != evento.turno) {
                error = "El tipo o turno no coincide con la solicitud";
                return false;
            }

            if (it->second.fase != FaseSolicitud::Activa) {
                error = "La hebra sale sin haber entrado";
                return false;
            }

            if (estado.escritoras_activas == 0) {
                error = "Una escritora sale sin estar activa";
                return false;
            }

            --estado.escritoras_activas;

            estado.solicitudes.erase(it);
        }

        // si no corresponde a ninguna de las acciones conocidas (Pide, Entra, Sale) se reporta error
        else {
            error = "Accion desconocida";
            return false;
        }

    }

    // si no corresponde a ninguna de las hebras definidas (Lectora, Escritora) se reporta error
    else {
        error = "Tipo de hebra desconocido";
        return false;
    }

    // si el evento fue procesado correctamente, se retorna true
    return true;
}

static ResultadoVerificacion verificar_interna(
    const std::vector<AnotacionTraza>& traza,
    const Politica* politica
)
{
    EstadoTraza estado;
    std::string error;

    long evidencia = 0;
    long siguiente_pedido_fifo = 0;
    long siguiente_entrada_fifo = 0;

    int lectora_inmediata = -1;
    bool esperar_entrada_inmediata = false;

    for (std::size_t i = 0; i < traza.size(); ++i) {
        const auto& evento = traza[i];
        bool sumar_evidencia = false;

        if (esperar_entrada_inmediata) {
            if (evento.hebra != lectora_inmediata ||
                evento.tipo != Tipo::Lectora ||
                evento.accion != Accion::Entra) {
                return {
                    false,
                    i,
                    "Una lectora habilitada no entro inmediatamente despues de Pide",
                    evidencia
                };
            }

            esperar_entrada_inmediata = false;
        }

        if (politica != nullptr) {
            if (*politica != Politica::Fifo && evento.turno != -1) {
                return {
                    false,
                    i,
                    "Las politicas de prioridad requieren turno -1",
                    evidencia
                };
            }

            switch (*politica) {
            case Politica::Lectores:
                if (evento.tipo == Tipo::Escritora &&
                    evento.accion == Accion::Entra &&
                    estado.lectoras_pendientes > 0) {
                    return {
                        false,
                        i,
                        "Una escritora entra con lectoras pendientes",
                        evidencia
                    };
                }

                if (evento.tipo == Tipo::Lectora &&
                    evento.accion == Accion::Entra &&
                    estado.escritoras_pendientes > 0) {
                    sumar_evidencia = true;
                }
                break;

            case Politica::Escritores:
                if (evento.tipo == Tipo::Lectora &&
                    evento.accion == Accion::Entra &&
                    estado.escritoras_pendientes > 0) {
                    return {
                        false,
                        i,
                        "Una lectora entra mientras hay escritoras pendientes",
                        evidencia
                    };
                }

                if (evento.tipo == Tipo::Lectora &&
                    evento.accion == Accion::Pide &&
                    estado.escritoras_pendientes > 0) {
                    sumar_evidencia = true;
                }
                break;

            case Politica::Fifo:
                if (evento.accion == Accion::Pide &&
                    evento.turno != siguiente_pedido_fifo) {
                    return {
                        false,
                        i,
                        "Turno FIFO de solicitud fuera de orden",
                        evidencia
                    };
                }

                if (evento.accion == Accion::Entra &&
                    evento.turno != siguiente_entrada_fifo) {
                    return {
                        false,
                        i,
                        "Turno FIFO de admision fuera de orden",
                        evidencia
                    };
                }

                if (evento.tipo == Tipo::Lectora &&
                    evento.accion == Accion::Entra &&
                    estado.lectoras_activas > 0) {
                    sumar_evidencia = true;
                }
                break;
            }
        }

        if (!aplicar_evento(estado, evento, error)) {
            return {false, i, error, evidencia};
        }

        if (sumar_evidencia) {
            ++evidencia;
        }

        if (politica != nullptr && *politica == Politica::Lectores &&
            evento.tipo == Tipo::Lectora &&
            evento.accion == Accion::Pide &&
            estado.escritoras_activas == 0) {
            esperar_entrada_inmediata = true;
            lectora_inmediata = evento.hebra;
        }

        if (politica != nullptr && *politica == Politica::Fifo) {
            if (evento.accion == Accion::Pide) {
                ++siguiente_pedido_fifo;
            }

            if (evento.accion == Accion::Entra) {
                ++siguiente_entrada_fifo;
            }
        }
    }

    if (esperar_entrada_inmediata) {
        return {
            false,
            traza.size(),
            "Falta la entrada inmediata de una lectora",
            evidencia
        };
    }

    if (!estado.solicitudes.empty() ||
        estado.lectoras_pendientes != 0 ||
        estado.escritoras_pendientes != 0 ||
        estado.lectoras_activas != 0 ||
        estado.escritoras_activas != 0) {
        return {
            false,
            traza.size(),
            "Traza incompleta, quedan solicitudes sin terminar",
            evidencia
        };
    }

    return {true, 0, "", evidencia};
}

// Verifica el vector de anotaciones con todos los eventos de la ejecucion
// y retorna el resultado de la validacion de la traza, incluyendo el indice del evento que fallo y un mensaje de error
ResultadoVerificacion verificar_traza(
    const std::vector<AnotacionTraza>& traza
)
{
    return verificar_interna(traza, nullptr);
}

ResultadoVerificacion verificar_traza(
    const std::vector<AnotacionTraza>& traza,
    Politica politica
)
{
    if (politica != Politica::Lectores &&
        politica != Politica::Escritores &&
        politica != Politica::Fifo) {
        throw std::invalid_argument("Politica desconocida para verificar la traza");
    }

    return verificar_interna(traza, &politica);
}
