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
else
EXT =
CRIAR_DIRETORIO = mkdir -p "$(BUILD_DIR)"
EXECUTAR_MATRIZ = "$(BUILD_DIR)/teste_matriz_adjacencia$(EXT)"
EXECUTAR_ALOCACAO = "$(BUILD_DIR)/teste_alocacao$(EXT)"
endif

.PHONY: all test sanitize

all: $(BUILD_DIR)/teste_matriz_adjacencia$(EXT) $(BUILD_DIR)/teste_alocacao$(EXT)

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

test: all
	$(EXECUTAR_MATRIZ)
	$(EXECUTAR_ALOCACAO)

# Usa outro diretorio para nao reutilizar objetos sem instrumentacao.
# Requer compilador e runtime com suporte a Address/Undefined Sanitizer.
sanitize:
	$(MAKE) BUILD_DIR=$(BUILD_DIR)/sanitizers CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all" test
