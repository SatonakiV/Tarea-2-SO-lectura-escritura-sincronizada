#include "verificador.hpp"

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

// Verifica el vector de anotaciones con todos los eventos de la ejecucion 
// y retorna el resultado de la validacion de la traza, incluyendo el indice del evento que fallo y un mensaje de error
ResultadoVerificacion verificar_traza(const std::vector<AnotacionTraza>& traza)
{
    EstadoTraza estado;
    std::string error;

    for (std::size_t i = 0; i < traza.size(); ++i) {

        // Si aplicar_evento retorna false, quiere decir que hubo un error en la traza
        // en tal caso se retorna el false indicando que la traza no es valida, indice i de la traza y  
        if (!aplicar_evento(estado, traza[i], error)) {
            return {false, i, error};
        }
    }

    if (!estado.solicitudes.empty() ||
        estado.lectoras_pendientes != 0 ||
        estado.escritoras_pendientes != 0 ||
        estado.lectoras_activas != 0 ||
        estado.escritoras_activas != 0) {

        return {false, traza.size(), "Traza incompleta, quedan solicitudes sin terminar"};
    }

    return {true, 0, ""};
}

