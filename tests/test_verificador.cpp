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

    std::cout << "Pruebas de verificador correctas\n";
    return 0;
}
