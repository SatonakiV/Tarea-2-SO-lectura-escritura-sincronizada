CXX ?= g++
CPPFLAGS += -Iinclude
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wpedantic -pthread
LDLIBS += -pthread
# Opcional: envoltorio por proceso para entornos cuyo ASLR impide arrancar TSan.
TSAN_RUNNER ?=
# Límite por proceso de pruebas; solo el arnés termina una ejecución bloqueada.
TEST_TIMEOUT ?= 60s

# P2/P3: añadir aquí los nuevos .cpp de sus módulos; nunca agregar otro main().
CORE_SOURCES = src/configuracion.cpp src/monitor_rw.cpp src/metricas.cpp \
               src/ejecucion.cpp src/fabrica.cpp src/integracion_monitor.cpp \
               src/integracion_atomico.cpp src/control_atomico_rw.cpp
HEADERS = $(wildcard include/*.hpp)

.PHONY: all test test-tsan clean
all: rw

rw: src/rw.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/rw.cpp $(CORE_SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

rw_tests: tests/pruebas.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/pruebas.cpp $(CORE_SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

test: rw rw_tests
	timeout --kill-after=5s $(TEST_TIMEOUT) ./rw_tests
	timeout --kill-after=5s $(TEST_TIMEOUT) python3 tests/probar_cli.py ./rw

# Ejecutable separado: no reutilizar un binario previamente compilado sin TSan.
test-tsan: tests/pruebas.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) -std=c++17 -O1 -g -Wall -Wextra -Wpedantic -pthread -fsanitize=thread tests/pruebas.cpp $(CORE_SOURCES) $(LDFLAGS) $(LDLIBS) -o rw_tests_tsan
	timeout --kill-after=5s $(TEST_TIMEOUT) $(TSAN_RUNNER) ./rw_tests_tsan

clean:
	$(RM) rw rw_tests rw_tests_tsan rw.exe rw_tests.exe rw_tests_tsan.exe
