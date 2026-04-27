/*
 * PROJETO 1 - MICROPROCESSADORES (MicPay)
 * Arquivo: main.c
 * Descricao: Logica principal da maquina de estados e fluxos de venda.
 */

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

// =======================================================
// FLUXO 1: VENDA À VISTA
// =======================================================
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

// =======================================================
// FLUXO 2: VENDA PARCELADA
// =======================================================
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

// =======================================================
// FLUXO 3: ESTORNO
// =======================================================
void fluxo_estorno() {
    char buffer_dados[10];

    // 1. Confirmação de segurança: Senha do Operador
    display_limpar();
    display_string("SENHA OPERADOR:");
    display_posiciona(1, 0);
    if (!le_dados_cliente(buffer_dados, 4, 1, 1)) return; // 4 dígitos, oculta, auto confirma

    if (strcmp(buffer_dados, "1254") != 0 && 
        strcmp(buffer_dados, "2349") != 0 && 
        strcmp(buffer_dados, "0738") != 0) {
        display_limpar();
        display_string("SENHA INVALIDA");
        _delay_ms(2000);
        return; // Aborta se a senha estiver errada
    }

    // 2. Pede o código de rastreio da venda
    display_limpar();
    display_string("CODIGO VENDA:");
    display_posiciona(1, 0);
    if (!le_dados_cliente(buffer_dados, 4, 0, 0)) return; // Confirma com '#'

    // 3. Pede o cartão para estornar
    display_limpar();
    display_string("NUMERO CARTAO:");
    display_posiciona(1, 0);
    if (!le_dados_cliente(buffer_dados, 6, 0, 0)) return; // Confirma com '#'

    // 4. Simula o processamento
    display_limpar();
    display_string("PROCESSANDO...");
    _delay_ms(1500); 
    
    display_limpar();
    display_string("ESTORNO OK!"); 
    _delay_ms(2000);
}

// =======================================================
// PROGRAMA PRINCIPAL
// =======================================================
int main(void) {
    configura_pinos_teclado();
    inicializa_display();
    
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
                    display_limpar();
                    display_off();
                    break;
                }

                if (strcmp(senha_op, "1254") == 0 || 
                    strcmp(senha_op, "2349") == 0 || 
                    strcmp(senha_op, "0738") == 0) {
                    display_limpar();
                    display_string("BEM-VINDO!"); 
                    _delay_ms(1000);
                    estado_atual = MENU_PRINCIPAL;
                } else {
                    display_limpar();
                    display_string("SENHA INVALIDA"); 
                    _delay_ms(2000);
                }
                break;

            case MENU_PRINCIPAL:
                display_limpar();
                display_string("1-VISTA 2-PARCEL"); // Linha 1 exata com 16 char
                display_posiciona(1, 0);
                display_string("3-ESTORNO *-SAIR"); // Linha 2 exata com 16 char

                char op = espera_tecla_com_shutdown();
                
                if (op == 'X' || op == '*') {
                    estado_atual = DESLIGADO;
                    display_limpar();
                    display_off();
                } 
                else if (op == '1') {
                    fluxo_venda_vista();
                }
                else if (op == '2') {
                    fluxo_venda_parcelada();
                }
                else if (op == '3') {
                    fluxo_estorno();
                }
                break;
        }
    }
    return 0;
}