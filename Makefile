CC ?= cc
PYTHON ?= python
CPPFLAGS = -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
BUILD_DIR ?= build

ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
EXT = .exe
CRIAR_DIRETORIO = if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
define EXECUTAR
"$(subst /,\,$(1))"
endef
else
EXT =
CRIAR_DIRETORIO = mkdir -p "$@"
define EXECUTAR
"$(1)"
endef
endif

GRAFOS = src/grafo_lista.c src/matriz_adjacencia.c
TRAVESSIAS = src/bfs.c src/dfs.c src/componentes_conexos.c
SIMULACAO = src/simulacao_falha.c
COMPLEMENTARES = src/algoritmos_complementares.c
DATASET = src/dataset.c
APP_FONTES = src/main.c src/metricas.c $(DATASET) $(GRAFOS) $(TRAVESSIAS) $(SIMULACAO) $(COMPLEMENTARES)

TESTES = $(BUILD_DIR)/teste_matriz_adjacencia$(EXT) \
	$(BUILD_DIR)/teste_alocacao$(EXT) \
	$(BUILD_DIR)/teste_grafo_lista$(EXT) \
	$(BUILD_DIR)/teste_grafo_lista_falhas$(EXT) \
	$(BUILD_DIR)/teste_bfs$(EXT) $(BUILD_DIR)/teste_bfs_falhas$(EXT) \
	$(BUILD_DIR)/teste_dfs$(EXT) $(BUILD_DIR)/teste_dfs_falhas$(EXT) \
	$(BUILD_DIR)/teste_componentes_conexos$(EXT) $(BUILD_DIR)/teste_componentes_falhas$(EXT) \
	$(BUILD_DIR)/teste_simulacao_falha$(EXT) $(BUILD_DIR)/teste_simulacao_falhas$(EXT) \
	$(BUILD_DIR)/teste_algoritmos_complementares$(EXT) $(BUILD_DIR)/teste_dataset$(EXT)

.PHONY: all app run test test-real test-bfs test-dfs test-componentes test-simulacao test-complementares test-falhas experimentos sanitize

all: app $(TESTES)
app: $(BUILD_DIR)/smart_grid$(EXT)

$(BUILD_DIR) resultados:
	$(CRIAR_DIRETORIO)

$(BUILD_DIR)/smart_grid$(EXT): $(APP_FONTES) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(APP_FONTES) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/teste_matriz_adjacencia$(EXT): src/matriz_adjacencia.c tests/teste_matriz_adjacencia.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/matriz_alocacao.o: src/matriz_adjacencia.c tests/teste_alocacao.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Dmalloc=teste_alocar -Dcalloc=teste_alocar_zerado -Dfree=teste_liberar -c src/matriz_adjacencia.c -o $@

$(BUILD_DIR)/teste_alocacao$(EXT): tests/teste_alocacao.c $(BUILD_DIR)/matriz_alocacao.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/teste_grafo_lista$(EXT): src/grafo_lista.c tests/test_grafo_lista.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/grafo_lista_falhas.o: src/grafo_lista.c tests/alocacao_teste.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -include tests/alocacao_teste.h -Dmalloc=teste_malloc -Drealloc=teste_realloc -Dfree=teste_free -c src/grafo_lista.c -o $@

$(BUILD_DIR)/teste_grafo_lista_falhas$(EXT): tests/test_grafo_lista.c $(BUILD_DIR)/grafo_lista_falhas.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -DGRAFO_LISTA_TESTAR_ALOCACAO $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/teste_bfs$(EXT): src/bfs.c tests/test_bfs.c $(GRAFOS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/bfs_falhas.o: src/bfs.c tests/alocacao_teste.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -include tests/alocacao_teste.h -Dmalloc=teste_malloc -Dfree=teste_free -c src/bfs.c -o $@

$(BUILD_DIR)/teste_bfs_falhas$(EXT): tests/test_bfs.c $(GRAFOS) $(BUILD_DIR)/bfs_falhas.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -DBFS_TESTAR_ALOCACAO $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/teste_dfs$(EXT): src/dfs.c tests/test_dfs.c $(GRAFOS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/dfs_falhas.o: src/dfs.c tests/alocacao_teste.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -include tests/alocacao_teste.h -Dmalloc=teste_malloc -Dfree=teste_free -c src/dfs.c -o $@

$(BUILD_DIR)/teste_dfs_falhas$(EXT): tests/test_dfs.c $(GRAFOS) $(BUILD_DIR)/dfs_falhas.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -DDFS_TESTAR_ALOCACAO $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/teste_componentes_conexos$(EXT): src/componentes_conexos.c src/bfs.c tests/test_componentes_conexos.c $(GRAFOS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/componentes_falhas.o: src/componentes_conexos.c tests/alocacao_teste.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -include tests/alocacao_teste.h -Dmalloc=teste_malloc -Dfree=teste_free -c src/componentes_conexos.c -o $@

$(BUILD_DIR)/teste_componentes_falhas$(EXT): tests/test_componentes_conexos.c $(GRAFOS) $(BUILD_DIR)/componentes_falhas.o $(BUILD_DIR)/bfs_falhas.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -DCOMPONENTES_TESTAR_ALOCACAO $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/teste_simulacao_falha$(EXT): $(SIMULACAO) src/componentes_conexos.c src/bfs.c tests/test_simulacao_falha.c $(GRAFOS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/simulacao_falhas.o: $(SIMULACAO) tests/alocacao_teste.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -include tests/alocacao_teste.h -Dmalloc=teste_malloc -Dfree=teste_free -c $(SIMULACAO) -o $@

$(BUILD_DIR)/teste_simulacao_falhas$(EXT): tests/test_simulacao_falha.c $(GRAFOS) $(BUILD_DIR)/simulacao_falhas.o $(BUILD_DIR)/componentes_falhas.o $(BUILD_DIR)/bfs_falhas.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -DSIMULACAO_TESTAR_ALOCACAO $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/teste_algoritmos_complementares$(EXT): $(COMPLEMENTARES) tests/test_algoritmos_complementares.c $(GRAFOS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/teste_dataset$(EXT): tests/test_dataset.c $(DATASET) $(GRAFOS) $(TRAVESSIAS) $(SIMULACAO) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

test: all
	$(foreach t,$(TESTES),$(call EXECUTAR,$(t))$(newline))

define newline


endef

test-real: $(BUILD_DIR)/teste_dataset$(EXT)
	$(call EXECUTAR,$<)
test-bfs: $(BUILD_DIR)/teste_bfs$(EXT) $(BUILD_DIR)/teste_bfs_falhas$(EXT)
	$(call EXECUTAR,$(word 1,$^))
	$(call EXECUTAR,$(word 2,$^))
test-dfs: $(BUILD_DIR)/teste_dfs$(EXT) $(BUILD_DIR)/teste_dfs_falhas$(EXT)
	$(call EXECUTAR,$(word 1,$^))
	$(call EXECUTAR,$(word 2,$^))
test-componentes: $(BUILD_DIR)/teste_componentes_conexos$(EXT) $(BUILD_DIR)/teste_componentes_falhas$(EXT)
	$(call EXECUTAR,$(word 1,$^))
	$(call EXECUTAR,$(word 2,$^))
test-simulacao: $(BUILD_DIR)/teste_simulacao_falha$(EXT) $(BUILD_DIR)/teste_simulacao_falhas$(EXT)
	$(call EXECUTAR,$(word 1,$^))
	$(call EXECUTAR,$(word 2,$^))
test-complementares: $(BUILD_DIR)/teste_algoritmos_complementares$(EXT)
	$(call EXECUTAR,$<)
test-falhas: $(BUILD_DIR)/teste_alocacao$(EXT) $(BUILD_DIR)/teste_grafo_lista_falhas$(EXT) $(BUILD_DIR)/teste_bfs_falhas$(EXT) $(BUILD_DIR)/teste_dfs_falhas$(EXT) $(BUILD_DIR)/teste_componentes_falhas$(EXT) $(BUILD_DIR)/teste_simulacao_falhas$(EXT)
	$(foreach t,$^,$(call EXECUTAR,$(t))$(newline))

run: app
	$(call EXECUTAR,$(BUILD_DIR)/smart_grid$(EXT))
experimentos: app | resultados
	$(call EXECUTAR,$(BUILD_DIR)/smart_grid$(EXT)) --experimentos
	$(PYTHON) scripts/gerar_graficos.py
sanitize:
	$(MAKE) BUILD_DIR=$(BUILD_DIR)/sanitizers CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all" test
