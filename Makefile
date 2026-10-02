CC = cc
CPPFLAGS = -Iinclude
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
BUILD_DIR = build

# GNU Make em Linux/macOS; mingw32-make em Windows (sem exigir Bash).
ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
EXT = .exe
CRIAR_DIRETORIO = if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
EXECUTAR_MATRIZ = "$(subst /,\,$(BUILD_DIR)/teste_matriz_adjacencia$(EXT))"
EXECUTAR_ALOCACAO = "$(subst /,\,$(BUILD_DIR)/teste_alocacao$(EXT))"
EXECUTAR_LISTA = "$(subst /,\,$(BUILD_DIR)/teste_grafo_lista$(EXT))"
EXECUTAR_LISTA_FALHAS = "$(subst /,\,$(BUILD_DIR)/teste_grafo_lista_falhas$(EXT))"
EXECUTAR_DFS = "$(subst /,\,$(BUILD_DIR)/teste_dfs$(EXT))"
EXECUTAR_DFS_FALHAS = "$(subst /,\,$(BUILD_DIR)/teste_dfs_falhas$(EXT))"
EXECUTAR_BFS = "$(subst /,\,$(BUILD_DIR)/teste_bfs$(EXT))"
EXECUTAR_BFS_FALHAS = "$(subst /,\,$(BUILD_DIR)/teste_bfs_falhas$(EXT))"
EXECUTAR_COMPLEMENTARES = "$(subst /,\,$(BUILD_DIR)/teste_algoritmos_complementares$(EXT))"
else
EXT =
CRIAR_DIRETORIO = mkdir -p "$(BUILD_DIR)"
EXECUTAR_MATRIZ = "$(BUILD_DIR)/teste_matriz_adjacencia$(EXT)"
EXECUTAR_ALOCACAO = "$(BUILD_DIR)/teste_alocacao$(EXT)"
EXECUTAR_LISTA = "$(BUILD_DIR)/teste_grafo_lista$(EXT)"
EXECUTAR_LISTA_FALHAS = "$(BUILD_DIR)/teste_grafo_lista_falhas$(EXT)"
EXECUTAR_DFS = "$(BUILD_DIR)/teste_dfs$(EXT)"
EXECUTAR_DFS_FALHAS = "$(BUILD_DIR)/teste_dfs_falhas$(EXT)"
EXECUTAR_BFS = "$(BUILD_DIR)/teste_bfs$(EXT)"
EXECUTAR_BFS_FALHAS = "$(BUILD_DIR)/teste_bfs_falhas$(EXT)"
EXECUTAR_COMPLEMENTARES = "$(BUILD_DIR)/teste_algoritmos_complementares$(EXT)"
endif

.PHONY: all test test-bfs test-dfs test-complementares test-falhas sanitize

all: $(BUILD_DIR)/teste_matriz_adjacencia$(EXT) $(BUILD_DIR)/teste_alocacao$(EXT)
all: $(BUILD_DIR)/teste_grafo_lista$(EXT) $(BUILD_DIR)/teste_grafo_lista_falhas$(EXT)
all: $(BUILD_DIR)/teste_dfs$(EXT) $(BUILD_DIR)/teste_dfs_falhas$(EXT)
all: $(BUILD_DIR)/teste_bfs$(EXT) $(BUILD_DIR)/teste_bfs_falhas$(EXT)
all: $(BUILD_DIR)/teste_algoritmos_complementares$(EXT)

$(BUILD_DIR):
	$(CRIAR_DIRETORIO)

$(BUILD_DIR)/teste_matriz_adjacencia$(EXT): src/matriz_adjacencia.c tests/teste_matriz_adjacencia.c include/matriz_adjacencia.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/matriz_adjacencia.c tests/teste_matriz_adjacencia.c $(LDFLAGS) $(LDLIBS) -o $@

# Apenas este objeto usa alocadores controlados; o modulo de producao
# nao recebe callbacks nem condicoes especiais para permitir os testes.
$(BUILD_DIR)/matriz_alocacao.o: src/matriz_adjacencia.c include/matriz_adjacencia.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Dmalloc=teste_alocar -Dcalloc=teste_alocar_zerado -Dfree=teste_liberar -c src/matriz_adjacencia.c -o $@

$(BUILD_DIR)/teste_alocacao$(EXT): tests/teste_alocacao.c include/matriz_adjacencia.h $(BUILD_DIR)/matriz_alocacao.o
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/teste_alocacao.c $(BUILD_DIR)/matriz_alocacao.o $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/teste_grafo_lista$(EXT): src/grafo_lista.c tests/test_grafo_lista.c include/grafo_lista.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/grafo_lista.c tests/test_grafo_lista.c $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/grafo_lista_falhas.o: src/grafo_lista.c include/grafo_lista.h tests/alocacao_teste.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -include tests/alocacao_teste.h -Dmalloc=teste_malloc -Drealloc=teste_realloc -Dfree=teste_free -c src/grafo_lista.c -o $@

$(BUILD_DIR)/teste_grafo_lista_falhas$(EXT): tests/test_grafo_lista.c tests/alocacao_teste.h $(BUILD_DIR)/grafo_lista_falhas.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -DGRAFO_LISTA_TESTAR_ALOCACAO tests/test_grafo_lista.c $(BUILD_DIR)/grafo_lista_falhas.o $(LDFLAGS) $(LDLIBS) -o $@

BFS_HEADERS = include/bfs.h include/grafo_lista.h include/matriz_adjacencia.h
BFS_GRAFOS = src/grafo_lista.c src/matriz_adjacencia.c

$(BUILD_DIR)/teste_bfs$(EXT): src/bfs.c tests/test_bfs.c $(BFS_GRAFOS) $(BFS_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/bfs.c $(BFS_GRAFOS) tests/test_bfs.c $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/bfs_falhas.o: src/bfs.c $(BFS_HEADERS) tests/alocacao_teste.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -include tests/alocacao_teste.h -Dmalloc=teste_malloc -Dfree=teste_free -c src/bfs.c -o $@

$(BUILD_DIR)/teste_bfs_falhas$(EXT): tests/test_bfs.c $(BFS_GRAFOS) $(BFS_HEADERS) tests/alocacao_teste.h $(BUILD_DIR)/bfs_falhas.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -DBFS_TESTAR_ALOCACAO $(BFS_GRAFOS) tests/test_bfs.c $(BUILD_DIR)/bfs_falhas.o $(LDFLAGS) $(LDLIBS) -o $@

DFS_HEADERS = include/dfs.h include/grafo_lista.h include/matriz_adjacencia.h
DFS_GRAFOS = src/grafo_lista.c src/matriz_adjacencia.c

$(BUILD_DIR)/teste_dfs$(EXT): src/dfs.c tests/test_dfs.c $(DFS_GRAFOS) $(DFS_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/dfs.c $(DFS_GRAFOS) tests/test_dfs.c $(LDFLAGS) $(LDLIBS) -o $@

# Reaproveita a instrumentacao de alocacao ja usada pela lista.
$(BUILD_DIR)/dfs_falhas.o: src/dfs.c $(DFS_HEADERS) tests/alocacao_teste.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -include tests/alocacao_teste.h -Dmalloc=teste_malloc -Dfree=teste_free -c src/dfs.c -o $@

$(BUILD_DIR)/teste_dfs_falhas$(EXT): tests/test_dfs.c $(DFS_GRAFOS) $(DFS_HEADERS) tests/alocacao_teste.h $(BUILD_DIR)/dfs_falhas.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -DDFS_TESTAR_ALOCACAO $(DFS_GRAFOS) tests/test_dfs.c $(BUILD_DIR)/dfs_falhas.o $(LDFLAGS) $(LDLIBS) -o $@

COMPLEMENTARES_HEADERS = include/algoritmos_complementares.h include/grafo_lista.h include/matriz_adjacencia.h

$(BUILD_DIR)/teste_algoritmos_complementares$(EXT): src/algoritmos_complementares.c tests/test_algoritmos_complementares.c $(DFS_GRAFOS) $(COMPLEMENTARES_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/algoritmos_complementares.c $(DFS_GRAFOS) tests/test_algoritmos_complementares.c $(LDFLAGS) $(LDLIBS) -o $@

test: all
	$(EXECUTAR_MATRIZ)
	$(EXECUTAR_ALOCACAO)
	$(EXECUTAR_LISTA)
	$(EXECUTAR_LISTA_FALHAS)
	$(EXECUTAR_BFS)
	$(EXECUTAR_BFS_FALHAS)
	$(EXECUTAR_DFS)
	$(EXECUTAR_DFS_FALHAS)
	$(EXECUTAR_COMPLEMENTARES)

test-bfs: $(BUILD_DIR)/teste_bfs$(EXT) $(BUILD_DIR)/teste_bfs_falhas$(EXT)
	$(EXECUTAR_BFS)
	$(EXECUTAR_BFS_FALHAS)

test-dfs: $(BUILD_DIR)/teste_dfs$(EXT) $(BUILD_DIR)/teste_dfs_falhas$(EXT)
	$(EXECUTAR_DFS)
	$(EXECUTAR_DFS_FALHAS)

test-complementares: $(BUILD_DIR)/teste_algoritmos_complementares$(EXT)
	$(EXECUTAR_COMPLEMENTARES)

test-falhas: $(BUILD_DIR)/teste_alocacao$(EXT) $(BUILD_DIR)/teste_grafo_lista_falhas$(EXT) $(BUILD_DIR)/teste_bfs_falhas$(EXT) $(BUILD_DIR)/teste_dfs_falhas$(EXT)
	$(EXECUTAR_ALOCACAO)
	$(EXECUTAR_LISTA_FALHAS)
	$(EXECUTAR_BFS_FALHAS)
	$(EXECUTAR_DFS_FALHAS)

# Usa outro diretorio para nao reutilizar objetos sem instrumentacao.
# Requer compilador e runtime com suporte a Address/Undefined Sanitizer.
sanitize:
	$(MAKE) BUILD_DIR=$(BUILD_DIR)/sanitizers CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all" test
