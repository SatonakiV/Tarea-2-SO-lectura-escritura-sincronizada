#pragma once

enum class Modo { Cond, Atomic };
enum class Politica { Lectores, Escritores, Fifo };
enum class Tipo { Lectora, Escritora };
enum class Accion { Pide, Entra, Sale };

struct AnotacionTraza {
    int hebra;
    Tipo tipo;
    Accion accion;
    long turno; // -1 en las políticas básicas; FIFO asignará desde cero (P2).
};

inline thread_local int id_hebra = -1;

// Punto de observación para pruebas y futura instrumentación de P2.
// Se invoca bajo el mutex del monitor: no esperar eventos de otras hebras,
// lanzar excepciones ni reentrar al monitor.
// Su vida debe abarcar todas las llamadas al monitor. No almacena la traza R6.
class ObservadorAdmision {
public:
    virtual ~ObservadorAdmision() = default;
    virtual void registrar(const AnotacionTraza& evento) noexcept = 0;
};
