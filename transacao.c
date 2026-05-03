#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include "display.h"
#include "teclado.h"
#include "transacao.h"

char espera_tecla_com_shutdown() {
	while(1) {
		// Primeiro, olhamos o hardware diretamente para a tecla *
		if (tecla_pressionada_bruta('*')) {
			uint16_t tempo = 0;
			uint8_t falhas = 0; // Filtro contra vibração física da mola do botão
			
			// Loop de 3 segundos exatos (300 * 10ms = 3000ms)
			while (tempo < 300) {
				if (tecla_pressionada_bruta('*')) {
					falhas = 0;
					} else {
					falhas++;
					// Se falhar por mais de 50ms (5 loops), o dedo saiu de verdade
					if (falhas > 5) break;
				}
				tempo++;
				_delay_ms(10);
			}
			
			// Se bateu 3 segundos cravados, DESLIGA NA HORA!
			if (tempo >= 300) {
				return 'X';
				} else {
				// Se foi toque rápido, espera soltar totalmente pra não dar duplo clique no menu
				while(tecla_pressionada_bruta('*')) _delay_ms(10);
				return '*';
			}
		}
		else {
			// Se não for o *, deixa a função lenta original cuidar dos números
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
		
		if (tecla == 'X') return 0;
		if (tecla == '*') return 0;

		if (tecla == '#' && indice > 0) {
			return 1;
		}

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

void fluxo_venda_vista() {
	char buffer_dados[10];

	display_limpar();
	display_string("VALOR VENDA:");
	display_posiciona(1, 0);
	display_string("R$ ");
	if (!le_dados_cliente(buffer_dados, 6, 0, 0)) return;

	display_limpar();
	display_string("1-DEB 2-CRED");
	display_posiciona(1, 0);
	char tipo = 0;
	while(tipo != '1' && tipo != '2') {
		tipo = espera_tecla_com_shutdown();
		if (tipo == '*' || tipo == 'X') return;
	}

	display_limpar();
	display_string("NUMERO CARTAO:");
	display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_dados, 6, 0, 0)) return;

	display_limpar();
	display_string("SENHA CLIENTE:");
	display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_dados, 6, 1, 0)) return;

	display_limpar();
	display_string("PROCESSANDO...");
	_delay_ms(1500);
	
	display_limpar();
	display_string("APROVADO!");
	_delay_ms(2000);
}

void fluxo_venda_parcelada() {
	char buffer_dados[10];

	display_limpar();
	display_string("VALOR VENDA:");
	display_posiciona(1, 0);
	display_string("R$ ");
	if (!le_dados_cliente(buffer_dados, 6, 0, 0)) return;

	display_limpar();
	display_string("QTD PARCELAS:");
	display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_dados, 1, 0, 0)) return;
	
	if (buffer_dados[0] < '1' || buffer_dados[0] > '3') {
		display_limpar();
		display_string("MAX 3 PARCELAS!");
		_delay_ms(2000);
		return;
	}

	display_limpar();
	display_string("NUMERO CARTAO:");
	display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_dados, 6, 0, 0)) return;

	display_limpar();
	display_string("SENHA CLIENTE:");
	display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_dados, 6, 1, 0)) return;

	display_limpar();
	display_string("PROCESSANDO...");
	_delay_ms(1500);
	
	display_limpar();
	display_string("APROVADO!");
	_delay_ms(2000);
}

void fluxo_estorno() {
	char buffer_dados[10];

	display_limpar();
	display_string("SENHA OPERADOR:");
	display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_dados, 4, 1, 1)) return;

	if (strcmp(buffer_dados, "1254") != 0 &&
	strcmp(buffer_dados, "2349") != 0 &&
	strcmp(buffer_dados, "0738") != 0) {
		display_limpar();
		display_string("SENHA INVALIDA");
		_delay_ms(2000);
		return;
	}

	display_limpar();
	display_string("CODIGO VENDA:");
	display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_dados, 4, 0, 0)) return;

	display_limpar();
	display_string("NUMERO CARTAO:");
	display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_dados, 6, 0, 0)) return;

	display_limpar();
	display_string("PROCESSANDO...");
	_delay_ms(1500);
	
	display_limpar();
	display_string("ESTORNO OK!");
	_delay_ms(2000);
}