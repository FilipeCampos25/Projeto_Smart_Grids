CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g
BUILD_DIR = build
EXEEXT =
ifeq ($(OS),Windows_NT)
EXEEXT = .exe
endif

.PHONY: all test test-falhas sanitize
all: $(BUILD_DIR)/teste_grafo_lista$(EXEEXT)

$(BUILD_DIR):
	mkdir -p "$@"

$(BUILD_DIR)/grafo_lista.o: src/grafo_lista.c include/grafo_lista.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/teste_grafo_lista$(EXEEXT): tests/test_grafo_lista.c $(BUILD_DIR)/grafo_lista.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) -o $@

test: $(BUILD_DIR)/teste_grafo_lista$(EXEEXT)
	./$(BUILD_DIR)/teste_grafo_lista$(EXEEXT)

$(BUILD_DIR)/grafo_lista_falhas.o: src/grafo_lista.c include/grafo_lista.h tests/alocacao_teste.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -include tests/alocacao_teste.h -Dmalloc=teste_malloc -Drealloc=teste_realloc -Dfree=teste_free -c $< -o $@

$(BUILD_DIR)/teste_grafo_lista_falhas$(EXEEXT): tests/test_grafo_lista.c tests/alocacao_teste.h $(BUILD_DIR)/grafo_lista_falhas.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -DGRAFO_LISTA_TESTAR_ALOCACAO tests/test_grafo_lista.c $(BUILD_DIR)/grafo_lista_falhas.o $(LDFLAGS) -o $@

test-falhas: $(BUILD_DIR)/teste_grafo_lista_falhas$(EXEEXT)
	./$(BUILD_DIR)/teste_grafo_lista_falhas$(EXEEXT)

# Requer GCC/Clang com ASan, UBSan e LeakSanitizer (por exemplo, Linux).
sanitize:
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $(MAKE) BUILD_DIR=$(BUILD_DIR)/sanitize CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer" LDFLAGS="$(LDFLAGS) -fsanitize=address,undefined" test test-falhas
