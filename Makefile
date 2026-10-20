# Makefile — atalhos para testes no Mac e compilação dos firmwares.
#
#   make test          compila e executa os testes do núcleo no computador
#   make firmware      compila transmissor e receptor para o ESP32 (arduino-cli)
#   make upload-tx PORTA=/dev/cu.usbserial-XXX   grava o transmissor
#   make upload-rx PORTA=/dev/cu.usbserial-XXX   grava o receptor
#   make upload-fips PORTA=/dev/cu.usbserial-XXX grava a validação FIPS-197
#   make clean         remove artefatos de compilação

LIB      := libraries/AES128Core
SRC      := $(wildcard $(LIB)/src/*.c)
BUILD    := build
CFLAGS   := -std=c99 -Wall -Wextra -Wpedantic -Werror -O2 -I$(LIB)/src

FQBN     := esp32:esp32:esp32:UploadSpeed=460800
PORTA    ?= /dev/cu.usbserial-130
ARDUINO  := arduino-cli

.PHONY: test firmware upload-tx upload-rx upload-fips clean

test: $(BUILD)/testes
	./$(BUILD)/testes

$(BUILD)/testes: tests/host/test_main.c tests/host/teste.h $(SRC) $(wildcard $(LIB)/src/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ tests/host/test_main.c $(SRC)

firmware:
	$(ARDUINO) compile --fqbn $(FQBN) --library $(LIB) --build-path $(BUILD)/transmissor firmware/transmissor
	$(ARDUINO) compile --fqbn $(FQBN) --library $(LIB) --build-path $(BUILD)/receptor firmware/receptor

upload-tx:
	$(ARDUINO) compile --fqbn $(FQBN) --library $(LIB) --build-path $(BUILD)/transmissor -u -p $(PORTA) firmware/transmissor

upload-rx:
	$(ARDUINO) compile --fqbn $(FQBN) --library $(LIB) --build-path $(BUILD)/receptor -u -p $(PORTA) firmware/receptor

upload-fips:
	$(ARDUINO) compile --fqbn $(FQBN) --library $(LIB) --build-path $(BUILD)/validacao_fips -u -p $(PORTA) firmware/validacao_fips

clean:
	rm -rf $(BUILD)
