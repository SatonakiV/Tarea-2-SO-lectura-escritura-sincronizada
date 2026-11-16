#pragma once
#include <limits>

struct Registro {
    long version = 0;
    long doble_version = 0;
};

// Precondición: acceso admitido por ControlRW; el controlador aporta la
// sincronización. No agregar un mutex aquí: serializaría a las lectoras.
inline void escribir_registro(Registro& registro) noexcept {
    ++registro.version;
    registro.doble_version = 2 * registro.version;
}
inline bool registro_consistente(const Registro& registro) noexcept {
    const long version = registro.version;
    const long doble = registro.doble_version;
    return version >= 0 && version <= std::numeric_limits<long>::max() / 2 &&
        doble == 2 * version;
}
