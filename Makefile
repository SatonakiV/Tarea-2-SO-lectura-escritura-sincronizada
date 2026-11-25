CXX ?= g++
CPPFLAGS += -Iinclude
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wpedantic -pthread
LDLIBS += -pthread
# Opcional: envoltorio por proceso para entornos cuyo ASLR impide arrancar TSan.
TSAN_RUNNER ?=
# Límite por proceso de pruebas; solo el arnés termina una ejecución bloqueada.
TEST_TIMEOUT ?= 60s
# Experimentos E1/E2 (P4). REPS=5 reproduce el plan; RESULTADOS cambia la carpeta.
PYTHON ?= python3
REPS ?= 5
RESULTADOS ?= resultados

# P2/P3: añadir aquí los nuevos .cpp de sus módulos; nunca agregar otro main().
CORE_SOURCES = src/configuracion.cpp src/monitor_rw.cpp src/metricas.cpp \
               src/ejecucion.cpp src/fabrica.cpp src/integracion_monitor.cpp \
               src/integracion_atomico.cpp src/control_atomico_rw.cpp \
               src/verificador.cpp
HEADERS = $(wildcard include/*.hpp)

.PHONY: all test test-tsan clean experimentos e1 e2 graficos
all: rw

rw: src/rw.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/rw.cpp $(CORE_SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

rw_tests: tests/pruebas.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/pruebas.cpp $(CORE_SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

rw_verificador_tests: tests/test_verificador.cpp src/verificador.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_verificador.cpp src/verificador.cpp $(LDFLAGS) $(LDLIBS) -o $@

rw_fifo_tests: tests/test_fifo.cpp src/monitor_rw.cpp src/verificador.cpp src/integracion_monitor.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_fifo.cpp src/monitor_rw.cpp src/verificador.cpp src/integracion_monitor.cpp $(LDFLAGS) $(LDLIBS) -o $@

test: rw rw_tests rw_verificador_tests rw_fifo_tests
	timeout --kill-after=5s $(TEST_TIMEOUT) ./rw_tests
	timeout --kill-after=5s $(TEST_TIMEOUT) ./rw_verificador_tests
	timeout --kill-after=5s $(TEST_TIMEOUT) ./rw_fifo_tests
	timeout --kill-after=5s $(TEST_TIMEOUT) python3 tests/probar_cli.py ./rw

# Ejecutable separado: no reutilizar un binario previamente compilado sin TSan.
test-tsan: tests/pruebas.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) -std=c++17 -O1 -g -Wall -Wextra -Wpedantic -pthread -fsanitize=thread tests/pruebas.cpp $(CORE_SOURCES) $(LDFLAGS) $(LDLIBS) -o rw_tests_tsan
	timeout --kill-after=5s $(TEST_TIMEOUT) $(TSAN_RUNNER) ./rw_tests_tsan

# E1 (100 ejecuciones con traza) y E2 (50 con notraza). Si E1 detecta un fallo,
# make se detiene antes de medir: no se miden versiones con fallos conocidos.
experimentos: e1 e2 graficos

e1: rw
	$(PYTHON) experimentos/e1.py --rw ./rw --reps $(REPS) --salida $(RESULTADOS)/e1

e2: rw
	$(PYTHON) experimentos/e2.py --rw ./rw --reps $(REPS) --salida $(RESULTADOS)/e2

graficos:
	$(PYTHON) experimentos/graficos.py --entrada $(RESULTADOS)/e2/promedios.csv --salida $(RESULTADOS)/e2/graficos

# No borra resultados/: son los datos originales de los experimentos.
clean:
	$(RM) rw rw_tests rw_verificador_tests rw_fifo_tests rw_tests_tsan rw.exe rw_tests.exe rw_verificador_tests.exe rw_fifo_tests.exe rw_tests_tsan.exe
