#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "display.h"
#include "teclado.h"

typedef enum { DESLIGADO, LOGIN, MENU_PRINCIPAL } t_estado;
t_estado estado_atual = DESLIGADO;

char espera_tecla_com_shutdown() {
	uint16_t tempo_off = 0;
	while(1) {
		if (tecla_pressionada_bruta('*')) {
			tempo_off++;
			_delay_ms(10);
			if (tempo_off >= 400) return 'X';
			} else {
			tempo_off = 0;
			char t = le_tecla();
			if (t != 0) return t;
		}
	}
}

int main(void) {
	configura_pinos_teclado();
	inicializa_display();
	
	while(1) {
		switch(estado_atual) {
			
			case DESLIGADO:
			if (tecla_pressionada_bruta('#')) {
				uint16_t tempo = 0;
				while (tecla_pressionada_bruta('#') && tempo < 300) {
					tempo++; _delay_ms(10);
				}
				if (tempo >= 300) {
					display_on();
					estado_atual = LOGIN;
					while(tecla_pressionada_bruta('#'));
				}
			}
			break;

			case LOGIN:
			display_limpar();
			display_string("SENHA ACESSO:");
			display_posiciona(1, 0);
			
			char senha[5] = {0};
			uint8_t i = 0;
			while(i < 4) {
				char t = espera_tecla_com_shutdown();
				if (t == 'X') { estado_atual = DESLIGADO; break; }
				
				if (t >= '0' && t <= '9') {
					senha[i++] = t;
					display_dado('*');
				}
			}

			if (estado_atual == DESLIGADO) { display_limpar(); display_off(); break; }

			if (strcmp(senha, "1254") == 0 || strcmp(senha, "2349") == 0 || strcmp(senha, "0738") == 0) {
				estado_atual = MENU_PRINCIPAL;
				} else {
				display_limpar();
				display_string("SENHA INVALIDA");
				_delay_ms(2000);
			}
			break;

			case MENU_PRINCIPAL:
			display_limpar();
			display_string("MicPay Pronto");
			display_posiciona(1, 0);
			display_string("1-Venda  *-Sair");

			char op = espera_tecla_com_shutdown();
			if (op == 'X') {
				estado_atual = DESLIGADO;
				display_limpar();
				display_off();
			}
			break;
		}
	}
	return 0;
}
