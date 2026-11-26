CXX ?= g++
CPPFLAGS += -Iinclude
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wpedantic -pthread
LDLIBS += -pthread
# Opcional: envoltorio por proceso para entornos cuyo ASLR impide arrancar TSan.
TSAN_RUNNER ?=
# Límite por proceso de pruebas; solo el arnés termina una ejecución bloqueada.
TEST_TIMEOUT ?= 60s
# Experimentos E1/E2 (P4). REPS=5 reproduce el plan. Cada corrida va por defecto
# a una carpeta nueva bajo material-local/ (ignorada por git), para no pisar los
# datos publicados en resultados/. Se fija una sola vez para que E1, E2 y los
# gráficos de una misma corrida compartan carpeta.
PYTHON ?= python3
REPS ?= 5
ifeq ($(origin RESULTADOS), undefined)
RESULTADOS := material-local/experimentos/$(shell date +%Y%m%d-%H%M%S)
endif

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

# E1 (100 ejecuciones con traza), luego E2 (50 con notraza), luego gráficos.
# Son pasos de una receta y no prerrequisitos, para que también con make -j
# corran en orden y se detengan si E1 falla: no se mide una versión con fallos.
experimentos: rw
	$(MAKE) --no-print-directory e1 RESULTADOS='$(RESULTADOS)'
	$(MAKE) --no-print-directory e2 RESULTADOS='$(RESULTADOS)'
	$(MAKE) --no-print-directory graficos RESULTADOS='$(RESULTADOS)'
	@echo "Resultados en $(RESULTADOS)"

e1: rw
	$(PYTHON) experimentos/e1.py --rw ./rw --reps $(REPS) --salida '$(RESULTADOS)/e1'

e2: rw
	$(PYTHON) experimentos/e2.py --rw ./rw --reps $(REPS) --salida '$(RESULTADOS)/e2'

graficos:
	@test -f '$(RESULTADOS)/e2/promedios.csv' || { echo "No existe $(RESULTADOS)/e2/promedios.csv: indique RESULTADOS=<carpeta de una corrida de E2>"; exit 1; }
	$(PYTHON) experimentos/graficos.py --entrada '$(RESULTADOS)/e2/promedios.csv' --salida '$(RESULTADOS)/e2/graficos'

# No borra resultados/ ni material-local/: son datos originales de los experimentos.
clean:
	$(RM) rw rw_tests rw_verificador_tests rw_fifo_tests rw_tests_tsan rw.exe rw_tests.exe rw_verificador_tests.exe rw_fifo_tests.exe rw_tests_tsan.exe
