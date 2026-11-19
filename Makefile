# Makefile del cliente de Google Sheets.
#
# Por ahora (paso T3 del Workplan) solo compila y prueba el módulo de red.

CC      = gcc
CFLAGS  = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2 -g
BUILD   = build

.PHONY: test test-red clean

# Tests sin internet (usan un servidor local)
test: $(BUILD)/test_net
	./$(BUILD)/test_net

# Prueba manual contra sheets.googleapis.com:443 (necesita internet)
test-red: $(BUILD)/test_net_real
	./$(BUILD)/test_net_real

$(BUILD)/test_net: tests/test_net.c src/net.c src/net.h | $(BUILD)
	$(CC) $(CFLAGS) -Isrc -o $@ tests/test_net.c src/net.c

$(BUILD)/test_net_real: tests/test_net_real.c src/net.c src/net.h | $(BUILD)
	$(CC) $(CFLAGS) -Isrc -o $@ tests/test_net_real.c src/net.c

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)