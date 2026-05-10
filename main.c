// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: main

#define F_CPU 16000000UL  // 16MHz, Unsigned (somente positivos) Long (32 bits)
#include <avr/io.h> // portas
#include <util/delay.h> // pausas e atrasos
#include <string.h> // textos
#include "display.h"
#include "teclado.h"
#include "transacao.h"
#include "relogio.h"
#include "admin_menu.h"
#include "serial.h"

typedef enum { DESLIGADO, LOGIN, MENU_PRINCIPAL, MENU_ADMIN } estados_sistema; // enum da nomes aos numeros inteiros
estados_sistema estado_atual = DESLIGADO; // sistema inicia com o display desligado

extern uint8_t pendencias_ativas;
uint8_t flag_checar_pendencias = 0; // alterado para iniciar em 0 (aguarda o horario certo)

int main(void) {
	configura_pinos_teclado();
	
	// configura D2 e D3 como saidas e forca inicio em 0V (desligados)
	DDRD |= (1 << PD2) | (1 << PD3);
	PORTD &= ~((1 << PD2) | (1 << PD3));

	inicializa_display();
	iniciar_relogio();
	conf_comunicacao();
	
	while(1) {
		
		if (estado_atual == DESLIGADO) {
			if (tecla_pressionada_bruta('#')) { // # = tecla para ligar o sistema
				uint16_t tempo = 0; // uint16_t para inteiro, positivo e 16 bits (necessario por ser 3seg, 300)
				while (tecla_pressionada_bruta('#') && tempo < 300) { // manter o botao de ligar pressionado por 3seg
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
			display_posiciona(1, 0); // 1 = linha inferior, 0 = primeira posicao a esquerda
			
			char senha_op[5]; // 4 digitos + \0
			if (!le_dados_cliente(senha_op, 4, 1, 1)) { // caso a leitura de dados falhe // 4 = num de digitos, 1 = ocultar com *, 1 = encerra a leitura logo que o 4o digito for pressionado
				estado_atual = DESLIGADO;
				display_limpar();
				display_off();
				} else {
				
				if (strcmp(senha_op, "0738") == 0) { // senha administrador
					char nome_op[17];
					fazer_login_servidor('9', nome_op);
					display_limpar(); display_string("OLA,");
					display_posiciona(1, 0); display_string(nome_op);
					_delay_ms(1500);
					estado_atual = MENU_ADMIN;
				}
				else if (strcmp(senha_op, "1254") == 0) { // senha operador 0
					if (operador_ativo(0)) {
						char nome_op[17];
						fazer_login_servidor('0', nome_op);
						display_limpar(); display_string("OLA,");
						display_posiciona(1, 0); display_string(nome_op);
						_delay_ms(1500);
						estado_atual = MENU_PRINCIPAL;
						} else {
						display_limpar(); display_string("OP BLOQUEADO"); _delay_ms(2000); // senha certa, mas acesso bloqueado pelo administrador
					}
				}
				else if (strcmp(senha_op, "2349") == 0) { // senha operador 1
					if (operador_ativo(1)) {
						char nome_op[17];
						fazer_login_servidor('1', nome_op);
						display_limpar(); display_string("OLA,");
						display_posiciona(1, 0); display_string(nome_op);
						_delay_ms(1500);
						estado_atual = MENU_PRINCIPAL;
						} else {
						display_limpar(); display_string("OP BLOQUEADO"); _delay_ms(2000); // senha certa, mas acesso bloqueado pelo administrador
					}
				}
				else {
					display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000); // quando inserido qualquer outra senha de 4 digitos nao valida
				}
			}
		}
		
		else if (estado_atual == MENU_ADMIN) {
			char res_admin = fluxo_menu_admin(); // admin_menu
			if (res_admin == 'X') { // caso o administrador desligue
				estado_atual = DESLIGADO;
				display_limpar(); display_off();
				} else {
				estado_atual = LOGIN; // caso o administrador volte ou saia
			}
		}
		
		else if (estado_atual == MENU_PRINCIPAL) { // operado por operador 0 ou 1
			
			display_limpar();
			display_string("1-VISTA 2-PARCEL"); // primeira linha do menu de vendas
			display_posiciona(1, 0); // desce o cursor
			display_string("3-ESTORNO *-SAIR"); // segunda linha do menu de vendas

			char op = 0;
			
			while(op == 0) { // laco de espera enquanto operador nao aperta nenhum botao
				if (flag_checar_pendencias) {
					flag_checar_pendencias = 0; // baixa a bandeira para nao verificar em loop
					
					if (verificar_existem_pendencias()) {
						PORTD |= (1 << PD2); // acende o led amarelo caso existam pendencias
						} else {
						PORTD &= ~(1 << PD2); // garante que ficara apagado senao tiver
					}

					if (pendencias_ativas) { // verifica se o adm deixou a funcao ON
						display_limpar();
						display_string("CHECANDO PEND..."); // aviso para operador
						_delay_ms(1500); // permanece aviso por 1.5seg
						processar_pendencias_servidor(); // executa a comunicacao
						
						// redesenha o menu principal
						display_limpar();
						display_string("1-VISTA 2-PARCEL");
						display_posiciona(1, 0);
						display_string("3-ESTORNO *-SAIR");
					}
				}

				if (tecla_pressionada_bruta('*')) {
					uint16_t tempo = 0;
					uint8_t falhas = 0;
					while (tempo < 300) { // precisa pressionar por 3seg
						if (tecla_pressionada_bruta('*')) falhas = 0; // se continua pressionado, zera as falhas
						else {
							falhas++; // soltou rapidamente = soma 1 falha
						if (falhas > 5) break; } // se soltou por mais de 50ms, assume que realmente tirou o dedo e aborta o cronometro
						tempo++;
						_delay_ms(10);
					}
					if (tempo >= 300) op = 'X'; // se segurou os 3s inteiros, atribui X para desligar a maquina
					else {
						while(tecla_pressionada_bruta('*')) _delay_ms(10); // se foi toque curto, trava aqui ate o botao subir fisicamente
						op = '*'; // atribui * para apenas fazer logoff e voltar ao menu
					}
					} else { // se nao era a tecla *, faz a leitura matricial normal
					char t = le_tecla();
					if (t != 0 && t != '*') op = t; // se alguma tecla util foi digitada, guarda na gaveta op
				}
				
				_delay_ms(10);
			}
			
			if (op == 'X') { // se pressionou X por 3seg, desliga
				estado_atual = DESLIGADO;
				display_limpar(); display_off();
			}
			else if (op == '*') estado_atual = LOGIN; // login
			else if (op == '1') fluxo_venda_vista(); // a vista
			else if (op == '2') fluxo_venda_parcelada(); // parcelado
			else if (op == '3') fluxo_estorno(); // estornar
		}
	}
	return 0;
}