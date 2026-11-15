#pragma once

#include "control_rw.hpp"
#include "metricas.hpp"

// Precondición: entrar_*() ya retornó exitosamente en esta hebra para este tipo.
// El guardián adopta esa admisión y es su único responsable de salida. El acceso
// al registro y el trabajo ocurren durante su vida, fuera del monitor.
class IntervaloAdmitido {
public:
    IntervaloAdmitido(ControlRW& control, Diagnostico& diagnostico, Tipo tipo) noexcept
        : control_(control), diagnostico_(diagnostico), tipo_(tipo) {
        diagnostico_.entrar(tipo_);
    }
    ~IntervaloAdmitido() noexcept {
        diagnostico_.salir(tipo_);
        if (tipo_ == Tipo::Lectora) control_.salir_lectura();
        else control_.salir_escritura();
    }
    IntervaloAdmitido(const IntervaloAdmitido&) = delete;
    IntervaloAdmitido& operator=(const IntervaloAdmitido&) = delete;
private:
    ControlRW& control_;
    Diagnostico& diagnostico_;
    Tipo tipo_;
};
