/*
 * PROJETO 1 - MICROPROCESSADORES (MicPay)
 * Arquivo: main.c
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "display.h"
#include "teclado.h"
#include "transacao.h"
#include "relogio.h"
#include "admin_menu.h"

typedef enum { DESLIGADO, LOGIN, MENU_PRINCIPAL, MENU_ADMIN } t_estado;
t_estado estado_atual = DESLIGADO;

int main(void) {
    configura_pinos_teclado();
    inicializa_display();
    relogio_init(); 
    
    while(1) {
        switch(estado_atual) {
            
            case DESLIGADO:
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
                break;

            case LOGIN:
                display_limpar();
                display_string("SENHA ACESSO:"); 
                display_posiciona(1, 0);
                
                char senha_op[5];
                if (!le_dados_cliente(senha_op, 4, 1, 1)) {
                    estado_atual = DESLIGADO;
                    display_limpar(); display_off();
                    break;
                }

                if (strcmp(senha_op, "0738") == 0) {
                    display_limpar(); display_string("ADMINISTRADOR"); _delay_ms(1000);
                    estado_atual = MENU_ADMIN;
                } 
                else if (strcmp(senha_op, "1254") == 0) {
                    if (is_operador_ativo(0)) {
                        display_limpar(); display_string("BEM-VINDO OP0!"); _delay_ms(1000);
                        estado_atual = MENU_PRINCIPAL;
                    } else {
                        display_limpar(); display_string("OP BLOQUEADO"); _delay_ms(2000);
                    }
                } 
                else if (strcmp(senha_op, "2349") == 0) {
                    if (is_operador_ativo(1)) {
                        display_limpar(); display_string("BEM-VINDO OP1!"); _delay_ms(1000);
                        estado_atual = MENU_PRINCIPAL;
                    } else {
                        display_limpar(); display_string("OP BLOQUEADO"); _delay_ms(2000);
                    }
                } 
                else {
                    display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000);
                }
                break;

            case MENU_ADMIN: { // <-- Chaves adicionadas aqui!
                char res_admin = fluxo_menu_admin();
                if (res_admin == 'X') {
                    estado_atual = DESLIGADO;
                    display_limpar(); display_off();
                } else {
                    estado_atual = LOGIN; 
                }
                break;
            } // <-- Fechamento das chaves aqui!

            case MENU_PRINCIPAL:
                display_limpar();
                display_string("1-VISTA 2-PARCEL"); 
                display_posiciona(1, 0);
                display_string("3-ESTORNO *-SAIR"); 

                char op = espera_tecla_com_shutdown();
                
                if (op == 'X') { 
                    estado_atual = DESLIGADO;
                    display_limpar(); display_off();
                } 
                else if (op == '*') { 
                    estado_atual = LOGIN;
                }
                else if (op == '1') fluxo_venda_vista();
                else if (op == '2') fluxo_venda_parcelada();
                else if (op == '3') fluxo_estorno();
                break;
        }
    }
    return 0;
}