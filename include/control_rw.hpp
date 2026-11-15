#pragma once

// Contrato del enunciado. Una entrada exitosa debe tener exactamente una salida
// del mismo tipo, en la misma hebra. El registro y las pausas quedan entre ambas.
// entrar_* puede fallar solo antes de admitir; salir_* no debe lanzar excepciones.
class ControlRW {
public:
    virtual ~ControlRW() = default;
    virtual void entrar_lectura() = 0;
    virtual void salir_lectura() = 0;
    virtual void entrar_escritura() = 0;
    virtual void salir_escritura() = 0;
};
