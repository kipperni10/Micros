// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: main

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
uint8_t flag_checar_pendencias = 0; // 0 = nada, 12/18/22 = horario que disparou

static char le_digito_login() {
	const char teclas_dig[3][3] = {
		{'1','2','3'},
		{'4','5','6'},
		{'7','8','9'},
	};

	// linhas 1 a 3 (D4-D6): digitos 1-9, todos os tres pinos de coluna
	for (int r = 0; r < 3; r++) {
		PORTD |= 0xF0;
		PORTD &= ~(1 << (r + 4));
		_delay_us(10);
		for (int c = 0; c < 3; c++) {
			if (!(PINC & (1 << c))) {
				if (debounce(c) == 0) {
					while (debounce(c) == 0);
					return teclas_dig[r][c];
				}
			}
		}
	}

	// linha 4 (D7): apenas '0' no PC1 (coluna 2)
	// PC0 (* ) e PC2 (#) sao intencionalmente ignorados
	PORTD |= 0xF0;
	PORTD &= ~(1 << PD7);
	_delay_us(10);
	if (!(PINC & (1 << PC1))) {
		if (debounce(1) == 0) {
			while (debounce(1) == 0);
			return '0';
		}
	}

	return 0; // nenhum digito pressionado
}

int main(void) {
	configura_pinos_teclado();

	// D2 = led amarelo (pagamento pendente), D3 = led vermelho (erro de conexao)
	DDRD  |=  (1 << PD2) | (1 << PD3);
	PORTD &= ~((1 << PD2) | (1 << PD3));

	inicializa_display();
	iniciar_relogio();
	conf_comunicacao();

	while (1) {

		// ---- DESLIGADO ----
		if (estado_atual == DESLIGADO) {
			if (tecla_pressionada_bruta('#')) {
				uint16_t tempo = 0;
				while (tecla_pressionada_bruta('#') && tempo < 300) { // liga com 3s no #
					tempo++;
					_delay_ms(10);
				}
				if (tempo >= 300) {
					display_on();
					estado_atual = LOGIN;
					while (tecla_pressionada_bruta('#')); // espera soltar antes de continuar
				}
			}
		}

		// ---- LOGIN ----
		else if (estado_atual == LOGIN) {
			display_limpar();
			display_string("SENHA ACESSO:");
			display_posiciona(1, 0);

			// leitura da senha com deteccao direta de * (mesmo padrao do MENU_PRINCIPAL)
			// garante que 4 segundos no * sempre desligam independente do contexto de entrada
			char senha_op[5];
			uint8_t idx_login = 0;
			senha_op[0] = '\0';
			uint8_t login_ok = 0;

			while (!login_ok) {
				if (tecla_pressionada_bruta('*')) {
					uint16_t tempo = 0;
					uint8_t falhas = 0;
					while (tempo < 400) { // 4 segundos para desligar
						if (tecla_pressionada_bruta('*')) falhas = 0;
						else { falhas++; if (falhas > 5) break; }
						tempo++;
						_delay_ms(10);
					}
					if (tempo >= 400) {
						estado_atual = DESLIGADO;
						display_limpar(); display_off();
						login_ok = 1; // sai do laco sem validar senha
						} else {
						while (tecla_pressionada_bruta('*')) _delay_ms(10);
						idx_login = 0; senha_op[0] = '\0'; // * curto: limpa digitos digitados
					}
					} else {
					char t = le_digito_login(); // nao bloqueia no *, diferente de le_tecla()
					if (t >= '0' && t <= '9' && idx_login < 4) {
						senha_op[idx_login++] = t;
						senha_op[idx_login] = '\0';
						display_dado('*'); // oculta senha com asterisco
						if (idx_login == 4) { _delay_ms(300); login_ok = 1; } // auto-confirma com 4 digitos
					}
				}
			}

			if (estado_atual != DESLIGADO) {

				if (strcmp(senha_op, "0738") == 0) { // administrador
					char nome_op[17];
					fazer_login_servidor('9', nome_op);
					display_limpar(); display_string("OLA,");
					display_posiciona(1, 0); display_string(nome_op);
					_delay_ms(1500);
					estado_atual = MENU_ADMIN;

					} else if (strcmp(senha_op, "1254") == 0) { // operador 0
					if (operador_ativo(0)) {
						char nome_op[17];
						fazer_login_servidor('0', nome_op);
						display_limpar(); display_string("OLA,");
						display_posiciona(1, 0); display_string(nome_op);
						_delay_ms(1500);
						estado_atual = MENU_PRINCIPAL;
						} else {
						display_limpar(); display_string("OP BLOQUEADO"); _delay_ms(2000);
					}

					} else if (strcmp(senha_op, "2349") == 0) { // operador 1
					if (operador_ativo(1)) {
						char nome_op[17];
						fazer_login_servidor('1', nome_op);
						display_limpar(); display_string("OLA,");
						display_posiciona(1, 0); display_string(nome_op);
						_delay_ms(1500);
						estado_atual = MENU_PRINCIPAL;
						} else {
						display_limpar(); display_string("OP BLOQUEADO"); _delay_ms(2000);
					}

					} else {
					display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000);
				}
			}
		}

		// ---- MENU ADMIN ----
		else if (estado_atual == MENU_ADMIN) {
			char res_admin = fluxo_menu_admin();
			if (res_admin == 'X') {
				estado_atual = DESLIGADO;
				display_limpar(); display_off();
				} else {
				estado_atual = LOGIN;
			}
		}

		// ---- MENU PRINCIPAL (operador 0 ou 1) ----
		else if (estado_atual == MENU_PRINCIPAL) {

			display_limpar();
			display_string("1-VISTA 2-PARCEL");
			display_posiciona(1, 0);
			display_string("3-ESTORNO *-SAIR");

			char op = 0;

			while (op == 0) {

				// verifica se ha parcelas pendentes para checar neste horario
				if (flag_checar_pendencias) {
					uint8_t hora_slot = flag_checar_pendencias; // salva qual horario disparou
					flag_checar_pendencias = 0;

					if (pendencias_ativas && verificar_existem_pendencias()) {
						display_limpar();
						display_string("CHECANDO PEND...");
						_delay_ms(1500);
						processar_pendencias_servidor(hora_slot);

						display_limpar();
						display_string("1-VISTA 2-PARCEL");
						display_posiciona(1, 0);
						display_string("3-ESTORNO *-SAIR");
					}
				}

				if (tecla_pressionada_bruta('*')) {
					uint16_t tempo = 0;
					uint8_t falhas = 0;

					while (tempo < 400) { // 4 segundos para desligar
						if (tecla_pressionada_bruta('*')) falhas = 0;
						else {
							falhas++;
							if (falhas > 5) break;
						}
						tempo++;
						_delay_ms(10);
					}

					if (tempo >= 400) op = 'X'; // desligar
					else {
						while (tecla_pressionada_bruta('*')) _delay_ms(10);
						op = '*'; // logoff
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

				} else if (op == '*') {
				estado_atual = LOGIN;

				} else if (op == '1') {
				fluxo_venda_vista();
				// checa se o desligamento foi solicitado de dentro da transacao
				if (flag_desligar) {
					flag_desligar = 0;
					estado_atual = DESLIGADO;
					display_limpar(); display_off();
				}

				} else if (op == '2') {
				fluxo_venda_parcelada();
				if (flag_desligar) {
					flag_desligar = 0;
					estado_atual = DESLIGADO;
					display_limpar(); display_off();
				}

				} else if (op == '3') {
				fluxo_estorno();
				if (flag_desligar) {
					flag_desligar = 0;
					estado_atual = DESLIGADO;
					display_limpar(); display_off();
				}
			}
		}
	}

	return 0;
}