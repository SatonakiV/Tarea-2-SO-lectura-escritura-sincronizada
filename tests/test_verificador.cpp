
#include <cassert>
#include <iostream>
#include <vector>

#include "verificador.hpp"

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
    assert(resultado.valida);

    auto invalida = valida;
    invalida[3] = {2, Tipo::Escritora, Accion::Entra, -1};

    resultado = verificar_traza(invalida);
    assert(!resultado.valida);
    assert(resultado.indice == 3);

    const std::vector<AnotacionTraza> incompleta = {
        {1, Tipo::Lectora, Accion::Pide, -1},
        {1, Tipo::Lectora, Accion::Entra, -1}
    };

    resultado = verificar_traza(incompleta);
    assert(!resultado.valida);
    assert(resultado.indice == incompleta.size());

    const std::vector<AnotacionTraza> hebra_incorrecta = {
        {1, Tipo::Lectora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Entra, -1}
    };

    resultado = verificar_traza(hebra_incorrecta);
    assert(!resultado.valida);
    assert(resultado.indice == 1);

    std::cout << "Pruebas de verificador correctas\n";
}
