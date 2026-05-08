#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include "display.h"
#include "teclado.h"
#include "transacao.h"
#include "relogio.h"
#include "admin_menu.h"

// Variáveis de estado global (1 = Ativo, 0 = Bloqueado)
uint8_t op0_ativo = 1;
uint8_t op1_ativo = 1;
uint8_t pendencias_ativas = 1;

uint8_t is_operador_ativo(uint8_t op_id) {
	if (op_id == 0) return op0_ativo;
	if (op_id == 1) return op1_ativo;
	return 0;
}

void menu_config_operadores() {
	display_limpar();
	display_string("1-OP0: "); display_string(op0_ativo ? "ON" : "OFF");
	display_posiciona(1, 0);
	display_string("2-OP1: "); display_string(op1_ativo ? "ON" : "OFF");
	
	char op = espera_tecla_com_shutdown();
	if (op == '1') op0_ativo = !op0_ativo;
	if (op == '2') op1_ativo = !op1_ativo;
	
	if (op == '1' || op == '2') {
		display_limpar(); display_string("SALVO!"); _delay_ms(1000);
	}
}

void menu_config_hora() {
	char buffer[5];
	display_limpar();
	display_string("NOVA HORA(HHMM):");
	display_posiciona(1, 0);
	
	if (le_dados_cliente(buffer, 4, 0, 1)) {
		uint8_t h = (buffer[0]-'0')*10 + (buffer[1]-'0');
		uint8_t m = (buffer[2]-'0')*10 + (buffer[3]-'0');
		
		if (h < 24 && m < 60) {
			ajustar_relogio(h, m, 0);
			display_limpar(); display_string("HORA SALVA!");
			} else {
			display_limpar(); display_string("HORA INVALIDA!");
		}
		_delay_ms(1500);
	}
}

void menu_config_pendencias() {
	display_limpar();
	display_string("1-VER PENDENCIAS");
	display_posiciona(1, 0);
	display_string("2-MODO: "); display_string(pendencias_ativas ? "ON" : "OFF");
	
	char op = espera_tecla_com_shutdown();
	if (op == '2') {
		pendencias_ativas = !pendencias_ativas;
		display_limpar(); display_string("SALVO!"); _delay_ms(1000);
		} else if (op == '1') {
		exibir_pendencias_admin();
	}
}

char fluxo_menu_admin() {
	while(1) {
		display_limpar();
		display_string("1-OP 2-HR"); // Linha 0 (Sobra espaço certinho pro relógio no canto)
		display_posiciona(1, 0);
		display_string("3-PND 4-SALDO"); // Linha 1
		
		char op = 0;
		uint8_t tempo_refresh = 0;

		while(op == 0) {
			tempo_refresh++;
			if (tempo_refresh >= 10) {
				tempo_refresh = 0;
				char hora_atual[6];
				leitura_horas(hora_atual);
				display_posiciona(0, 11); // Escreve o HH:MM bem no canto superior direito
				display_string(hora_atual);
			}

			if (tecla_pressionada_bruta('*')) {
				uint16_t tempo = 0;
				uint8_t falhas = 0;
				
				while (tempo < 300) {
					if (tecla_pressionada_bruta('*')) {
						falhas = 0;
						} else {
						falhas++;
						if (falhas > 5) break;
					}
					tempo++;
					_delay_ms(10);
				}
				
				if (tempo >= 300) op = 'X';
				else {
					while(tecla_pressionada_bruta('*')) _delay_ms(10);
					op = '*';
				}
			}
			else {
				char t = le_tecla();
				if (t != 0 && t != '*') op = t;
			}
			
			if (op == 0) _delay_ms(20);
		}
		
		// Embora não apareça na tela, a tecla * continua servindo para voltar ao login
		if (op == '*') return '*';
		if (op == 'X') return 'X';
		
		if (op == '1') menu_config_operadores();
		if (op == '2') menu_config_hora();
		if (op == '3') menu_config_pendencias();
		if (op == '4') relatorio_saldos_locais();
	}
}