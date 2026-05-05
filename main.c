// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: main

#define F_CPU 16000000UL // 16MHz, Unsigned (somente positivos) Long (32 bits)
#include <avr/io.h> // portas
#include <util/delay.h> // pausas e atrasos
#include <string.h> // textos
#include "display.h"
#include "teclado.h"
#include "transacao.h"
#include "relogio.h"
#include "admin_menu.h"

typedef enum { DESLIGADO, LOGIN, MENU_PRINCIPAL, MENU_ADMIN } estados_sistema; // enum da nomes aos números inteiros
estados_sistema estado_atual = DESLIGADO; // sistema inicia com o display desligado

int main(void) {
    configura_pinos_teclado();
    inicializa_display();
    relogio_init(); 
    
    while(1) {
        
        if (estado_atual == DESLIGADO) {
            if (tecla_pressionada_bruta('#')) { // # = tecla para ligar o sistema
                uint16_t tempo = 0; // uint16_t para inteiro, positivo e 16 bits (necessário por ser 3seg, 300)
                while (tecla_pressionada_bruta('#') && tempo < 300) { // manter o botão de ligar pressionado por 3seg
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
            display_posiciona(1, 0); // 1 = linha inferior, 0 = primeira posição à esquerda
            
            char senha_op[5]; // 4 dígitos + \0
            if (!le_dados_cliente(senha_op, 4, 1, 1)) { // caso a leitura de dados falhe // 4 = n° de dígitos, 1 = ocultar com *, 1 = encerra a leitura logo que o 4° dígito for pressionado
                estado_atual = DESLIGADO;
                display_limpar(); 
                display_off();
            } else {
                if (strcmp(senha_op, "0738") == 0) { // senha administrador
                    display_limpar(); display_string("ADMINISTRADOR"); _delay_ms(1000);
                    estado_atual = MENU_ADMIN;
                } 
                else if (strcmp(senha_op, "1254") == 0) { // senha operador 0
                    if (is_operador_ativo(0)) {
                        display_limpar(); display_string("BEM-VINDO OP0!"); _delay_ms(1000); 
                        estado_atual = MENU_PRINCIPAL;
                    } else {
                        display_limpar(); display_string("OP BLOQUEADO"); _delay_ms(2000); // senha certa, mas acesso bloqueado pelo administrador 
                    }
                } 
                else if (strcmp(senha_op, "2349") == 0) { // senha operador 1
                    if (is_operador_ativo(1)) {
                        display_limpar(); display_string("BEM-VINDO OP1!"); _delay_ms(1000);
                        estado_atual = MENU_PRINCIPAL;
                    } else {
                        display_limpar(); display_string("OP BLOQUEADO"); _delay_ms(2000); // senha certa, mas acesso bloqueado pelo administrador 
                    }
                } 
                else {
                    display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000); // quando inserido qualquer outra senha de 4 dígitos não válida
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
            display_string("1-VISTA 2-PARCEL"); 
            display_posiciona(1, 0); 
            display_string("3-ESTORNO *-SAIR"); 

            char op = espera_tecla_com_shutdown(); // congela a tela no menu principal até ter uma ação
            
            if (op == 'X') { // desligar
                estado_atual = DESLIGADO;
                display_limpar(); display_off();
            } 
            else if (op == '*') { // cancelar
                estado_atual = LOGIN;
            }
            else if (op == '1') fluxo_venda_vista(); // a vista
            else if (op == '2') fluxo_venda_parcelada(); // parcelado
            else if (op == '3') fluxo_estorno(); // estornar
        }
    }
    return 0;
}
