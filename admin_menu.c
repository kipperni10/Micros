// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: admin_menu

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

uint8_t op0_ativo = 1;
uint8_t op1_ativo = 1;
uint8_t pendencias_ativas = 1;

extern uint8_t flag_checar_pendencias;

uint8_t operador_ativo(uint8_t id_operador) {
    if (id_operador == 0) return op0_ativo;
    if (id_operador == 1) return op1_ativo;
    return 0;
}

void menu_config_operadores() {
    display_limpar();
    display_string("1-OP0: ");
    if (op0_ativo) display_string("ON"); else display_string("OFF");

    display_posiciona(1, 0);
    display_string("2-OP1: ");
    if (op1_ativo) display_string("ON"); else display_string("OFF");

    char op = espera_tecla(); // trava esperando o adm escolher (tambem monitora * por 4s)

    if (op == '1') op0_ativo = !op0_ativo;
    if (op == '2') op1_ativo = !op1_ativo;

    if (op == '1' || op == '2') {
        display_limpar(); display_string("SALVO!"); _delay_ms(1000);
    }
    // se op == 'X', flag_desligar ja foi setada em espera_tecla; a funcao retorna normalmente
    // e fluxo_menu_admin() checa a flag logo apos o retorno
}

// configura data e hora em sequencia na mesma operacao de menu
void menu_config_hora() {
    char buffer[7]; // buffer reutilizado: cabe ate 6 digitos + \0

    // ---- passo 1: data ----
    display_limpar();
    display_string("DATA (DDMMAA):");
    display_posiciona(1, 0);

    if (!le_dados_cliente(buffer, 6, 0, 1)) return; // auto-confirma ao 6o digito

    uint8_t d  = (buffer[0] - '0') * 10 + (buffer[1] - '0');
    uint8_t m  = (buffer[2] - '0') * 10 + (buffer[3] - '0');
    uint8_t a  = (buffer[4] - '0') * 10 + (buffer[5] - '0');

    if (d < 1 || d > 31 || m < 1 || m > 12) {
        display_limpar(); display_string("DATA INVALIDA!"); _delay_ms(1500); return;
    }

    // ---- passo 2: hora ----
    display_limpar();
    display_string("HORA (HHMM):");
    display_posiciona(1, 0);

    if (!le_dados_cliente(buffer, 4, 0, 1)) return; // auto-confirma ao 4o digito

    uint8_t h  = (buffer[0] - '0') * 10 + (buffer[1] - '0');
    uint8_t mi = (buffer[2] - '0') * 10 + (buffer[3] - '0');

    if (h >= 24 || mi >= 60) {
        display_limpar(); display_string("HORA INVALIDA!"); _delay_ms(1500); return;
    }

    // ambos validos: salva data e hora e zera os segundos
    ajustar_data(d, m, a);
    ajustar_relogio(h, mi, 0);
    display_limpar(); display_string("DATA/HORA SALVA!"); _delay_ms(1500);
}

void menu_config_pendencias() {
    display_limpar();
    display_string("1-VER PENDENCIAS");
    display_posiciona(1, 0);
    display_string("2-MODO: ");
    if (pendencias_ativas) display_string("ON"); else display_string("OFF");

    char op = espera_tecla();

    if (op == '2') {
        pendencias_ativas = !pendencias_ativas;
        display_limpar(); display_string("SALVO!"); _delay_ms(1000);
    } else if (op == '1') {
        exibir_pendencias_admin();
    }
    // op == 'X': flag_desligar setada em espera_tecla, fluxo_menu_admin trata
}

char fluxo_menu_admin() {
    while (1) {
        display_limpar();
        display_string("1-OP 2-HR");
        display_posiciona(1, 0);
        display_string("3-PND 4-SALDO");

        char op = 0;
        uint8_t tempo_refresh = 0;

        while (op == 0) {
            tempo_refresh++;
            if (tempo_refresh >= 10) { // atualiza relogio a cada ~200ms
                tempo_refresh = 0;
                char hora_atual[6];
                leitura_horas(hora_atual);
                display_posiciona(0, 11);
                display_string(hora_atual);
            }

            // verifica se ha pendencias para checar neste horario
            if (flag_checar_pendencias) {
                uint8_t hora_slot = flag_checar_pendencias; // salva qual horario disparou
                flag_checar_pendencias = 0;

                if (pendencias_ativas && verificar_existem_pendencias()) {
                    display_limpar();
                    display_string("CHECANDO PEND...");
                    _delay_ms(1500);
                    processar_pendencias_servidor(hora_slot);
                    display_limpar();
                    display_string("1-OP 2-HR");
                    display_posiciona(1, 0);
                    display_string("3-PND 4-SALDO");
                    tempo_refresh = 9; // forca atualizacao do relogio no proximo ciclo
                }
            }

            if (tecla_pressionada_bruta('*')) {
                uint16_t tempo = 0;
                uint8_t falhas = 0;

                while (tempo < 400) { // 4 segundos
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

            if (op == 0) _delay_ms(20);
        }

        if (op == '*') return '*'; // logoff
        if (op == 'X') return 'X'; // desligar

        if (op == '1') menu_config_operadores();
        // checa se o desligamento foi solicitado de dentro da sub-funcao
        if (flag_desligar) { flag_desligar = 0; return 'X'; }

        if (op == '2') menu_config_hora();
        if (flag_desligar) { flag_desligar = 0; return 'X'; }

        if (op == '3') menu_config_pendencias();
        if (flag_desligar) { flag_desligar = 0; return 'X'; }

        if (op == '4') relatorio_saldos_locais();
        if (flag_desligar) { flag_desligar = 0; return 'X'; }
    }
}
