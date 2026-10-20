# Makefile — atalhos para testes no Mac e compilação dos firmwares.
#
#   make test          compila e executa os testes do núcleo no computador
#   make analise       gera no computador o relatório de análise da dupla
#   make firmware      compila transmissor e receptor para o ESP32 (arduino-cli)
#   make upload-tx PORTA=/dev/cu.usbserial-XXX   grava o transmissor
#   make upload-rx PORTA=/dev/cu.usbserial-XXX   grava o receptor
#   make upload-fips PORTA=/dev/cu.usbserial-XXX grava a validação FIPS-197
#   make upload-analise PORTA=/dev/cu.usbserial-XXX grava o relatório de análise
#   make upload-teste-uart PORTA=/dev/cu.usbserial-XXX grava o teste da UART2
#   make clean         remove artefatos de compilação

LIB      := libraries/AES128Core
SRC      := $(wildcard $(LIB)/src/*.c)
BUILD    := build
CFLAGS   := -std=c99 -Wall -Wextra -Wpedantic -Werror -O2 -I$(LIB)/src

FQBN     := esp32:esp32:esp32:UploadSpeed=460800
PORTA    ?= /dev/cu.usbserial-130
ARDUINO  := arduino-cli

.PHONY: test analise firmware upload-tx upload-rx upload-fips upload-analise upload-teste-uart clean

test: $(BUILD)/testes
	./$(BUILD)/testes

$(BUILD)/testes: tests/host/test_main.c tests/host/teste.h $(SRC) $(wildcard $(LIB)/src/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ tests/host/test_main.c $(SRC)

analise: $(BUILD)/analise
	./$(BUILD)/analise

$(BUILD)/analise: tests/host/analise_host.c $(SRC) $(wildcard $(LIB)/src/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ tests/host/analise_host.c $(SRC)

firmware:
	$(ARDUINO) compile --fqbn $(FQBN) --library $(LIB) --build-path $(BUILD)/transmissor firmware/transmissor
	$(ARDUINO) compile --fqbn $(FQBN) --library $(LIB) --build-path $(BUILD)/receptor firmware/receptor

upload-tx:
	$(ARDUINO) compile --fqbn $(FQBN) --library $(LIB) --build-path $(BUILD)/transmissor -u -p $(PORTA) firmware/transmissor

upload-rx:
	$(ARDUINO) compile --fqbn $(FQBN) --library $(LIB) --build-path $(BUILD)/receptor -u -p $(PORTA) firmware/receptor

upload-fips:
	$(ARDUINO) compile --fqbn $(FQBN) --library $(LIB) --build-path $(BUILD)/validacao_fips -u -p $(PORTA) firmware/validacao_fips

upload-analise:
	$(ARDUINO) compile --fqbn $(FQBN) --library $(LIB) --build-path $(BUILD)/analise_fw -u -p $(PORTA) firmware/analise

upload-teste-uart:
	$(ARDUINO) compile --fqbn $(FQBN) --build-path $(BUILD)/teste_uart -u -p $(PORTA) firmware/teste_uart

clean:
	rm -rf $(BUILD)
