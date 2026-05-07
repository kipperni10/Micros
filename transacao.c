#define F_CPU 16000000UL // 16MHz
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h> // Adicionado para usar o sprintf
#include "display.h"
#include "teclado.h"
#include "transacao.h"

typedef struct {
	char numero[7];
	int32_t saldo_centavos;
} CartaoLocal;

CartaoLocal banco_local[5] = {
	{"111111", 80000},
	{"222222", 80000},
	{"333333", 80000},
	{"444444", 80000},
	{"555555", 80000}
};

char fila_servidor_cartao_local[7] = "";
int32_t fila_servidor_valor_local = 0;

char espera_tecla_com_shutdown() {
	while(1) {
		if (tecla_pressionada_bruta('*')) {
			uint16_t tempo = 0;
			uint8_t falhas = 0;
			while (tempo < 300) {
				if (tecla_pressionada_bruta('*')) falhas = 0;
				else { falhas++; if (falhas > 5) break; }
				tempo++;
				_delay_ms(10);
			}
			if (tempo >= 300) return 'X';
			else {
				while(tecla_pressionada_bruta('*')) _delay_ms(10);
				return '*';
			}
		}
		else {
			char t = le_tecla();
			if (t != 0 && t != '*') return t;
		}
	}
}

uint8_t le_dados_cliente(char* buffer, uint8_t tamanho_maximo, uint8_t ocultar, uint8_t auto_submit) {
	uint8_t indice = 0;
	buffer[0] = '\0';
	while(1) {
		char tecla = espera_tecla_com_shutdown();
		if (tecla == 'X' || tecla == '*') return 0;
		if (tecla == '#' && indice > 0) return 1;
		
		if (tecla >= '0' && tecla <= '9' && indice < tamanho_maximo) {
			buffer[indice] = tecla;
			indice++;
			buffer[indice] = '\0';
			if (ocultar) display_dado('*');
			else display_dado(tecla);
			if (auto_submit && indice == tamanho_maximo) {
				_delay_ms(300);
				return 1;
			}
		}
	}
}

void relatorio_saldos_locais() {
	for(int i = 0; i < 5; i++) {
		char msg_saldo[16];
		int32_t reais = banco_local[i].saldo_centavos / 100;
		int32_t centavos = banco_local[i].saldo_centavos % 100;
		sprintf(msg_saldo, "R$ %ld,%02ld", reais, centavos);

		display_limpar();
		display_string("CARTAO ");
		display_string(banco_local[i].numero);
		display_posiciona(1, 0);
		display_string(msg_saldo);
		
		// Espera qualquer tecla pra ir pro proximo cartao
		char op = 0;
		while (op == 0) {
			op = espera_tecla_com_shutdown();
			if (op == 'X') return; // sai de tudo se segurar *
		}
	}
	display_limpar();
	display_string("FIM RELATORIO");
	_delay_ms(1500);
}

void fluxo_venda_vista() {
	char buffer_valor[10];
	char buffer_bandeira[2];
	char buffer_cartao[10];
	char buffer_senha[10];

	display_limpar(); display_string("VALOR VENDA:"); display_posiciona(1, 0); display_string("R$ ");
	if (!le_dados_cliente(buffer_valor, 6, 0, 0)) return;
	int32_t valor_venda = atol(buffer_valor);

	display_limpar(); display_string("1-DEB 2-CRED"); display_posiciona(1, 0);
	char tipo_pagamento = 0;
	while(tipo_pagamento != '1' && tipo_pagamento != '2') {
		tipo_pagamento = espera_tecla_com_shutdown();
		if (tipo_pagamento == '*' || tipo_pagamento == 'X') return;
	}

	display_limpar(); display_string("BANDEIRA (0-9):"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_bandeira, 1, 0, 1)) return;
	char bandeira = buffer_bandeira[0];

	display_limpar(); display_string("NUMERO CARTAO:"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_cartao, 6, 0, 0)) return;

	display_limpar(); display_string("SENHA CLIENTE:"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_senha, 6, 1, 0)) return;

	display_limpar(); display_string("PROCESSANDO..."); _delay_ms(1000);

	if (bandeira == '0') {
		uint8_t cartao_encontrado = 0;
		for(int i = 0; i < 5; i++) {
			if (strcmp(banco_local[i].numero, buffer_cartao) == 0) {
				cartao_encontrado = 1;
				if (banco_local[i].saldo_centavos >= valor_venda) {
					banco_local[i].saldo_centavos -= valor_venda;
					strcpy(fila_servidor_cartao_local, buffer_cartao);
					fila_servidor_valor_local = valor_venda;

					char msg_saldo[16];
					int32_t reais = banco_local[i].saldo_centavos / 100;
					int32_t centavos = banco_local[i].saldo_centavos % 100;
					sprintf(msg_saldo, "R$ %ld,%02ld", reais, centavos);

					display_limpar(); display_string("LOCAL APROVADO"); _delay_ms(1500);
					display_limpar(); display_string("NOVO SALDO:"); display_posiciona(1, 0); display_string(msg_saldo); _delay_ms(2500);
					} else {
					display_limpar(); display_string("SALDO INSUFIC."); _delay_ms(2000);
				}
				break;
			}
		}
		if (!cartao_encontrado) { display_limpar(); display_string("CARTAO INVALIDO"); _delay_ms(2000); }
		} else {
		display_limpar(); display_string("EXTERNO APROVADO"); _delay_ms(2000);
	}
}

void fluxo_venda_parcelada() {
	char buffer_valor[10];
	char buffer_bandeira[2];
	char buffer_cartao[10];
	char buffer_senha[10];
	char buffer_parcelas[2];

	display_limpar(); display_string("VALOR VENDA:"); display_posiciona(1, 0); display_string("R$ ");
	if (!le_dados_cliente(buffer_valor, 6, 0, 0)) return;
	int32_t valor_venda = atol(buffer_valor);

	display_limpar(); display_string("QTD PARCELAS:"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_parcelas, 1, 0, 0)) return;
	if (buffer_parcelas[0] < '1' || buffer_parcelas[0] > '3') {
		display_limpar(); display_string("MAX 3 PARCELAS!"); _delay_ms(2000); return;
	}

	display_limpar(); display_string("BANDEIRA (0-9):"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_bandeira, 1, 0, 1)) return;
	char bandeira = buffer_bandeira[0];

	display_limpar(); display_string("NUMERO CARTAO:"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_cartao, 6, 0, 0)) return;

	display_limpar(); display_string("SENHA CLIENTE:"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_senha, 6, 1, 0)) return;

	display_limpar(); display_string("PROCESSANDO..."); _delay_ms(1000);

	if (bandeira == '0') {
		uint8_t cartao_encontrado = 0;
		for(int i = 0; i < 5; i++) {
			if (strcmp(banco_local[i].numero, buffer_cartao) == 0) {
				cartao_encontrado = 1;
				if (banco_local[i].saldo_centavos >= valor_venda) {
					banco_local[i].saldo_centavos -= valor_venda;
					strcpy(fila_servidor_cartao_local, buffer_cartao);
					fila_servidor_valor_local = valor_venda;

					char msg_saldo[16];
					int32_t reais = banco_local[i].saldo_centavos / 100;
					int32_t centavos = banco_local[i].saldo_centavos % 100;
					sprintf(msg_saldo, "R$ %ld,%02ld", reais, centavos);

					display_limpar(); display_string("LOCAL APROVADO"); _delay_ms(1500);
					display_limpar(); display_string("NOVO SALDO:"); display_posiciona(1, 0); display_string(msg_saldo); _delay_ms(2500);
					} else {
					display_limpar(); display_string("SALDO INSUFIC."); _delay_ms(2000);
				}
				break;
			}
		}
		if (!cartao_encontrado) { display_limpar(); display_string("CARTAO INVALIDO"); _delay_ms(2000); }
		} else {
		display_limpar(); display_string("EXTERNO APROVADO"); _delay_ms(2000);
	}
}

void fluxo_estorno() {
	char buffer_dados[10];
	char buffer_bandeira[2];
	char buffer_cartao[10];

	display_limpar(); display_string("SENHA OPERADOR:"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_dados, 4, 1, 1)) return;

	if (strcmp(buffer_dados, "1254") != 0 && strcmp(buffer_dados, "2349") != 0 && strcmp(buffer_dados, "0738") != 0) {
		display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000); return;
	}

	display_limpar(); display_string("CODIGO VENDA:"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_dados, 4, 0, 0)) return;

	display_limpar(); display_string("BANDEIRA (0-9):"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_bandeira, 1, 0, 1)) return;
	char bandeira = buffer_bandeira[0];

	display_limpar(); display_string("NUMERO CARTAO:"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_cartao, 6, 0, 0)) return;

	display_limpar(); display_string("PROCESSANDO..."); _delay_ms(1500);

	if (bandeira == '0') {
		uint8_t cartao_encontrado = 0;
		for(int i = 0; i < 5; i++) {
			if (strcmp(banco_local[i].numero, buffer_cartao) == 0) {
				cartao_encontrado = 1;
				// Simula aprovação e mostra saldo (depois adicionamos historico de codigo de venda se precisar)
				char msg_saldo[16];
				int32_t reais = banco_local[i].saldo_centavos / 100;
				int32_t centavos = banco_local[i].saldo_centavos % 100;
				sprintf(msg_saldo, "R$ %ld,%02ld", reais, centavos);

				display_limpar(); display_string("ESTORNO LOCAL OK"); _delay_ms(1500);
				display_limpar(); display_string("SALDO:"); display_posiciona(1, 0); display_string(msg_saldo); _delay_ms(2500);
				break;
			}
		}
		if (!cartao_encontrado) { display_limpar(); display_string("CARTAO INVALIDO"); _delay_ms(2000); }
		} else {
		display_limpar(); display_string("ESTORNO EXT OK"); _delay_ms(2000);
	}
}