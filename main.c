#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "display.h"
#include "teclado.h"
#include "transacao.h"
#include "relogio.h"
#include "admin_menu.h"
#include "serial.h"

typedef enum { DESLIGADO, LOGIN, MENU_PRINCIPAL, MENU_ADMIN } estados_sistema;
estados_sistema estado_atual = DESLIGADO;

extern uint8_t pendencias_ativas;

int main(void) {
	configura_pinos_teclado();
	
	DDRD |= (1 << PD2) | (1 << PD3);
	PORTD &= ~((1 << PD2) | (1 << PD3));

	inicializa_display();
	iniciar_relogio();
	serial_init();
	
	while(1) {
		
		if (estado_atual == DESLIGADO) {
			if (tecla_pressionada_bruta('#')) {
				uint16_t tempo = 0;
				while (tecla_pressionada_bruta('#') && tempo < 300) {
					tempo++;
					_delay_ms(10);
				}
				if (tempo >= 300) {
					display_on();
					estado_atual = LOGIN;
					while(tecla_pressionada_bruta('#'));
				}
			}
		}
		
		else if (estado_atual == LOGIN) {
			display_limpar();
			display_string("SENHA ACESSO:");
			display_posiciona(1, 0);
			
			char senha_op[5];
			if (!le_dados_cliente(senha_op, 4, 1, 1)) {
				estado_atual = DESLIGADO;
				display_limpar();
				display_off();
				} else {
				
				if (strcmp(senha_op, "0738") == 0) {
					char nome_op[17];
					fazer_login_servidor('9', nome_op);
					display_limpar(); display_string("OLA,");
					display_posiciona(1, 0); display_string(nome_op);
					_delay_ms(1500);
					estado_atual = MENU_ADMIN;
				}
				else if (strcmp(senha_op, "1254") == 0) {
					if (is_operador_ativo(0)) {
						char nome_op[17];
						fazer_login_servidor('0', nome_op);
						display_limpar(); display_string("OLA,");
						display_posiciona(1, 0); display_string(nome_op);
						_delay_ms(1500);
						estado_atual = MENU_PRINCIPAL;
						} else {
						display_limpar(); display_string("OP BLOQUEADO"); _delay_ms(2000);
					}
				}
				else if (strcmp(senha_op, "2349") == 0) {
					if (is_operador_ativo(1)) {
						char nome_op[17];
						fazer_login_servidor('1', nome_op);
						display_limpar(); display_string("OLA,");
						display_posiciona(1, 0); display_string(nome_op);
						_delay_ms(1500);
						estado_atual = MENU_PRINCIPAL;
						} else {
						display_limpar(); display_string("OP BLOQUEADO"); _delay_ms(2000);
					}
				}
				else {
					display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000);
				}
			}
		}
		
		else if (estado_atual == MENU_ADMIN) {
			char res_admin = fluxo_menu_admin();
			if (res_admin == 'X') {
				estado_atual = DESLIGADO;
				display_limpar(); display_off();
				} else {
				estado_atual = LOGIN;
			}
		}
		
		else if (estado_atual == MENU_PRINCIPAL) {
			
			display_limpar();
			display_string("1-VISTA 2-PARCEL");
			display_posiciona(1, 0);
			display_string("3-ESTORNO *-SAIR");

			char op = 0;
			
			while(op == 0) {
				if (flag_checar_pendencias) {
					flag_checar_pendencias = 0;
					if (pendencias_ativas) {
						display_limpar();
						display_string("CHECANDO PEND...");
						_delay_ms(1500);
						processar_pendencias_servidor();
						
						display_limpar();
						display_string("1-VISTA 2-PARCEL");
						display_posiciona(1, 0);
						display_string("3-ESTORNO *-SAIR");
					}
				}

				if (tecla_pressionada_bruta('*')) {
					uint16_t tempo = 0;
					uint8_t falhas = 0;
					while (tempo < 300) {
						if (tecla_pressionada_bruta('*')) falhas = 0;
						else { falhas++; if (falhas > 5) break; }
						tempo++;
						_delay_ms(10);
					}
					if (tempo >= 300) op = 'X';
					else {
						while(tecla_pressionada_bruta('*')) _delay_ms(10);
						op = '*';
					}
					} else {
					char t = le_tecla();
					if (t != 0 && t != '*') op = t;
				}
				
				_delay_ms(10);
			}
			
			if (op == 'X') {
				estado_atual = DESLIGADO;
				display_limpar(); display_off();
			}
			else if (op == '*') estado_atual = LOGIN;
			else if (op == '1') fluxo_venda_vista();
			else if (op == '2') fluxo_venda_parcelada();
			else if (op == '3') fluxo_estorno();
		}
	}
	return 0;
}