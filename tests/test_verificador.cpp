#include <iostream>
#include <vector>

#include "verificador.hpp"

bool comprobar(bool condicion, const char* mensaje) {
    if (!condicion) {
        std::cerr << "Fallo en el verificador: " << mensaje << '\n';
        return false;
    }
    return true;
}

bool comprobar_rechazo(
    const std::vector<AnotacionTraza>& traza,
    Politica politica,
    std::size_t indice,
    long evidencia,
    const char* mensaje
) {
    const auto resultado = verificar_traza(traza, politica);

    return comprobar(!resultado.valida, mensaje) &&
           comprobar(resultado.indice == indice,
                    "Indice de rechazo incorrecto") &&
           comprobar(resultado.evidencia == evidencia,
                    "Evidencia en rechazo incorrecta");
}

bool prueba_observaciones_revision() {
    const std::vector<AnotacionTraza> prioridad_lectores_valida = {
        {1, Tipo::Escritora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Entra, -1},
        {2, Tipo::Lectora, Accion::Sale, -1},
        {1, Tipo::Escritora, Accion::Entra, -1},
        {1, Tipo::Escritora, Accion::Sale, -1}
    };

    auto resultado = verificar_traza(
        prioridad_lectores_valida, Politica::Lectores
    );

    if (!comprobar(resultado.valida,
                  "prioridad de lectores valida rechazada")) return false;
    if (!comprobar(resultado.evidencia == 1,
                  "evidencia de prioridad de lectores incorrecta")) return false;

    const std::vector<AnotacionTraza> prioridad_lectores_invalida = {
        {1, Tipo::Escritora, Accion::Pide, -1},
        {1, Tipo::Escritora, Accion::Entra, -1},
        {2, Tipo::Lectora, Accion::Pide, -1},
        {3, Tipo::Escritora, Accion::Pide, -1},
        {1, Tipo::Escritora, Accion::Sale, -1},
        {3, Tipo::Escritora, Accion::Entra, -1}
    };

    if (!comprobar_rechazo(
            prioridad_lectores_invalida, Politica::Lectores, 5, 0,
            "escritora entro con lectora pendiente")) return false;

    const std::vector<AnotacionTraza> lectora_no_inmediata = {
        {1, Tipo::Escritora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Pide, -1},
        {3, Tipo::Escritora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Entra, -1}
    };

    if (!comprobar_rechazo(
            lectora_no_inmediata, Politica::Lectores, 2, 0,
            "lectora habilitada no entro inmediatamente")) return false;

    const std::vector<AnotacionTraza> prioridad_escritores_valida = {
        {1, Tipo::Escritora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Pide, -1},
        {1, Tipo::Escritora, Accion::Entra, -1},
        {1, Tipo::Escritora, Accion::Sale, -1},
        {2, Tipo::Lectora, Accion::Entra, -1},
        {2, Tipo::Lectora, Accion::Sale, -1}
    };

    resultado = verificar_traza(
        prioridad_escritores_valida, Politica::Escritores
    );

    if (!comprobar(resultado.valida,
                  "prioridad de escritores valida rechazada")) return false;
    if (!comprobar(resultado.evidencia == 1,
                  "evidencia de prioridad de escritores incorrecta")) return false;

    const std::vector<AnotacionTraza> prioridad_escritores_invalida = {
        {1, Tipo::Escritora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Entra, -1}
    };

    if (!comprobar_rechazo(
            prioridad_escritores_invalida, Politica::Escritores, 2, 1,
            "lectora entro con escritora pendiente")) return false;

    const std::vector<AnotacionTraza> pedido_duplicado = {
        {1, Tipo::Escritora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Pide, -1}
    };

    if (!comprobar_rechazo(
            pedido_duplicado, Politica::Escritores, 2, 1,
            "Pide duplicado incremento evidencia")) return false;

    const std::vector<AnotacionTraza> una_lectora = {
        {1, Tipo::Lectora, Accion::Pide, -1},
        {1, Tipo::Lectora, Accion::Entra, -1},
        {1, Tipo::Lectora, Accion::Sale, -1}
    };

    for (Politica politica : {Politica::Lectores, Politica::Escritores}) {
        for (std::size_t i = 0; i < una_lectora.size(); ++i) {
            auto alterada = una_lectora;
            alterada[i].turno = 7;

            if (!comprobar_rechazo(
                    alterada, politica, i, 0,
                    "turno distinto de -1 aceptado")) return false;
        }

        auto alterada = una_lectora;
        for (auto& evento : alterada) {
            evento.turno = 7;
        }

        if (!comprobar_rechazo(
                alterada, politica, 0, 0,
                "traza con turnos incorrectos aceptada")) return false;
    }

    const std::vector<AnotacionTraza> fifo_valida = {
        {1, Tipo::Lectora, Accion::Pide, 0},
        {1, Tipo::Lectora, Accion::Entra, 0},
        {2, Tipo::Lectora, Accion::Pide, 1},
        {2, Tipo::Lectora, Accion::Entra, 1},
        {1, Tipo::Lectora, Accion::Sale, 0},
        {2, Tipo::Lectora, Accion::Sale, 1}
    };

    resultado = verificar_traza(fifo_valida, Politica::Fifo);

    if (!comprobar(resultado.valida,
                  "traza FIFO valida rechazada")) return false;
    if (!comprobar(resultado.evidencia == 1,
                  "evidencia FIFO incorrecta")) return false;

    const std::vector<AnotacionTraza> fifo_sin_pedido = {
        {1, Tipo::Lectora, Accion::Pide, 0},
        {1, Tipo::Lectora, Accion::Entra, 0},
        {2, Tipo::Lectora, Accion::Entra, 1}
    };

    if (!comprobar_rechazo(
            fifo_sin_pedido, Politica::Fifo, 2, 0,
            "entrada FIFO sin Pide previo aceptada")) return false;

    return true;
}

int main() {
    const std::vector<AnotacionTraza> valida = {
        {1, Tipo::Lectora, Accion::Pide, -1},
        {1, Tipo::Lectora, Accion::Entra, -1},
        {2, Tipo::Escritora, Accion::Pide, -1},
        {1, Tipo::Lectora, Accion::Sale, -1},
        {2, Tipo::Escritora, Accion::Entra, -1},
        {2, Tipo::Escritora, Accion::Sale, -1}
    };

    auto resultado = verificar_traza(valida);
    if (!comprobar(resultado.valida, "traza valida rechazada")) return 1;

    auto invalida = valida;
    invalida[3] = {2, Tipo::Escritora, Accion::Entra, -1};

    resultado = verificar_traza(invalida);
    if (!comprobar(!resultado.valida, "escritora entro con lectora activa")) return 1;
    if (!comprobar(resultado.indice == 3, "indice incorrecto para escritora con lectora activa")) return 1;

    const std::vector<AnotacionTraza> incompleta = {
        {1, Tipo::Lectora, Accion::Pide, -1},
        {1, Tipo::Lectora, Accion::Entra, -1}
    };

    resultado = verificar_traza(incompleta);
    if (!comprobar(!resultado.valida, "traza incompleta aceptada")) return 1;
    if (!comprobar(resultado.indice == incompleta.size(), "indice incorrecto para traza incompleta")) return 1;

    const std::vector<AnotacionTraza> hebra_incorrecta = {
        {1, Tipo::Lectora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Entra, -1}
    };

    resultado = verificar_traza(hebra_incorrecta);
    if (!comprobar(!resultado.valida, "entrada sin solicitud previa aceptada")) return 1;
    if (!comprobar(resultado.indice == 1, "indice incorrecto para entrada sin solicitud")) return 1;

    const std::vector<AnotacionTraza> dos_lectoras = {
        {1, Tipo::Lectora, Accion::Pide, -1},
        {1, Tipo::Lectora, Accion::Entra, -1},
        {2, Tipo::Lectora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Entra, -1},
        {1, Tipo::Lectora, Accion::Sale, -1},
        {2, Tipo::Lectora, Accion::Sale, -1}
    };

    resultado = verificar_traza(dos_lectoras);
    if (!comprobar(resultado.valida, "dos lectoras activas fueron rechazadas")) return 1;

    const std::vector<AnotacionTraza> lectora_con_escritora = {
        {1, Tipo::Escritora, Accion::Pide, -1},
        {1, Tipo::Escritora, Accion::Entra, -1},
        {2, Tipo::Lectora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Entra, -1},
        {1, Tipo::Escritora, Accion::Sale, -1},
        {2, Tipo::Lectora, Accion::Sale, -1}
    };

    resultado = verificar_traza(lectora_con_escritora);
    if (!comprobar(!resultado.valida, "lectora entro con escritora activa")) return 1;
    if (!comprobar(resultado.indice == 3, "indice incorrecto para lectora con escritora activa")) return 1;

    const std::vector<AnotacionTraza> dos_escritoras = {
        {1, Tipo::Escritora, Accion::Pide, -1},
        {1, Tipo::Escritora, Accion::Entra, -1},
        {2, Tipo::Escritora, Accion::Pide, -1},
        {2, Tipo::Escritora, Accion::Entra, -1},
        {1, Tipo::Escritora, Accion::Sale, -1},
        {2, Tipo::Escritora, Accion::Sale, -1}
    };

    resultado = verificar_traza(dos_escritoras);
    if (!comprobar(!resultado.valida, "dos escritoras entraron simultaneamente")) return 1;
    if (!comprobar(resultado.indice == 3, "indice incorrecto para dos escritoras activas")) return 1;
    if (!prueba_observaciones_revision()) return 1;

    std::cout << "Pruebas de verificador correctas\n";
    return 0;
}
