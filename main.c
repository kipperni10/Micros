// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //
// Arquivo: main.c

#define F_CPU 16000000UL // 16MHz [cite: 208]
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "display.h"
#include "teclado.h"
#include "transacao.h"
#include "relogio.h"
#include "admin_menu.h"
#include "serial.h" // Inclusão do módulo de comunicação serial

typedef enum { DESLIGADO, LOGIN, MENU_PRINCIPAL, MENU_ADMIN } estados_sistema;
estados_sistema estado_atual = DESLIGADO;

int main(void) {
	configura_pinos_teclado();
	inicializa_display();
	relogio_init();
	serial_init(); // Inicializa a serial com taxa 19200 8-E-1 conforme o protocolo
	
	while(1) {
		
		if (estado_atual == DESLIGADO) {
			if (tecla_pressionada_bruta('#')) { // # = tecla para ligar o sistema [cite: 235]
				uint16_t tempo = 0;
				while (tecla_pressionada_bruta('#') && tempo < 300) { // manter pressionado por 3seg [cite: 235]
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
				// Verificação de Senhas e Notificação Serial [cite: 209]
				
				if (strcmp(senha_op, "0738") == 0) { // Senha administrador [cite: 255]
					// Envia ao servidor: 'M' 'L' '9' (Aviso de login Administrador)
					serial_envia_char('M'); serial_envia_char('L'); serial_envia_char('9');
					
					display_limpar();
					display_string("ADMINISTRADOR");
					_delay_ms(1000);
					estado_atual = MENU_ADMIN;
				}
				else if (strcmp(senha_op, "1254") == 0) { // Senha operador 0 [cite: 253]
					if (is_operador_ativo(0)) {
						// Envia ao servidor: 'M' 'L' '0' (Aviso de login Operador 0)
						serial_envia_char('M'); serial_envia_char('L'); serial_envia_char('0');
						
						display_limpar();
						display_string("BEM-VINDO OP0!");
						_delay_ms(1000);
						estado_atual = MENU_PRINCIPAL;
						} else {
						display_limpar();
						display_string("OP BLOQUEADO");
						_delay_ms(2000);
					}
				}
				else if (strcmp(senha_op, "2349") == 0) { // Senha operador 1 [cite: 254]
					if (is_operador_ativo(1)) {
						// Envia ao servidor: 'M' 'L' '1' (Aviso de login Operador 1)
						serial_envia_char('M'); serial_envia_char('L'); serial_envia_char('1');
						
						display_limpar();
						display_string("BEM-VINDO OP1!");
						_delay_ms(1000);
						estado_atual = MENU_PRINCIPAL;
						} else {
						display_limpar();
						display_string("OP BLOQUEADO");
						_delay_ms(2000);
					}
				}
				else {
					display_limpar();
					display_string("SENHA INVALIDA");
					_delay_ms(2000);
				}
			}
		}
		
		else if (estado_atual == MENU_ADMIN) {
			char res_admin = fluxo_menu_admin();
			if (res_admin == 'X') { // Desligar sistema
				estado_atual = DESLIGADO;
				display_limpar();
				display_off();
				} else {
				estado_atual = LOGIN; // Voltar para a tela de login
			}
		}
		
		else if (estado_atual == MENU_PRINCIPAL) {
			display_limpar();
			display_string("1-VISTA 2-PARCEL");
			display_posiciona(1, 0);
			display_string("3-ESTORNO *-SAIR");

			char op = espera_tecla_com_shutdown();
			
			if (op == 'X') { // Desligar sistema (Pressionar * por 3seg)
				estado_atual = DESLIGADO;
				display_limpar();
				display_off();
			}
			else if (op == '*') { // Logoff (Voltar para o Login)
				estado_atual = LOGIN;
			}
			else if (op == '1') fluxo_venda_vista();
			else if (op == '2') fluxo_venda_parcelada();
			else if (op == '3') fluxo_estorno();
		}
	}
	return 0;
}