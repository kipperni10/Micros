// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: transacao

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "display.h"
#include "teclado.h"
#include "transacao.h"
#include "serial.h"
#include "relogio.h"

// flag global: setada em 1 quando * e mantido 4 segundos dentro de qualquer funcao de transacao.
// o chamador (main.c ou admin_menu.c) deve checar e agir apos cada retorno de funcao.
uint8_t flag_desligar = 0;

// =============================================================================
// BANCO LOCAL (cartoes proprios do estabelecimento)
// =============================================================================

typedef struct {
    char numero[7];
    int32_t saldo_centavos; // saldo em centavos para evitar float
    uint8_t compras_acima_50; // contador de cashback
} CartaoLocal;

CartaoLocal banco_local[5] = {
    {"111111", 80000, 0},
    {"222222", 80000, 0},
    {"333333", 80000, 0},
    {"444444", 80000, 0},
    {"555555", 80000, 0}
};

// =============================================================================
// PENDENCIAS DE PARCELAS FUTURAS
// =============================================================================

#define MAX_PENDENCIAS 10

typedef struct {
    uint8_t status;     // 0 = livre, 1 = aguardando vencimento, 2 = inadimplente
    char bandeira;
    char cartao[7];
    char valor[10];     // valor da parcela individual em centavos (string)
    uint8_t dia_venc;   // data de vencimento desta parcela
    uint8_t mes_venc;
    uint8_t ano_venc;
    uint8_t tentou_12h; // 0 = ainda nao tentou, 1 = tentou e pagou, 2 = tentou e nao pagou
    uint8_t tentou_18h;
    uint8_t tentou_22h;
} Pendencia;

// arrays globais sao inicializados em zero pelo C, status 0 = livre por padrao
Pendencia lista_pendencias[MAX_PENDENCIAS];

// retorna 1 se existe alguma parcela ainda aguardando confirmacao de pagamento
uint8_t verificar_existem_pendencias() {
    for (int i = 0; i < MAX_PENDENCIAS; i++) {
        if (lista_pendencias[i].status == 1) return 1;
    }
    return 0;
}

// cria registros de pendencia para as parcelas futuras de uma venda parcelada.
// a primeira parcela ja foi cobrada pelo servidor no ato da venda, entao comeca de p=1.
void adicionar_pendencias_parceladas(char bandeira, char* cartao, char* valor_total_str, uint8_t num_parcelas) {
    int32_t valor_total   = atol(valor_total_str);
    int32_t valor_parcela = valor_total / num_parcelas; // divisao inteira em centavos
    char str_parcela[10];
    sprintf(str_parcela, "%ld", valor_parcela);

    uint8_t dia_base, mes_base, ano_base;
    leitura_data_numerica(&dia_base, &mes_base, &ano_base);

    for (uint8_t p = 1; p < num_parcelas; p++) {
        uint8_t dia_v, mes_v, ano_v;
        calcular_data_futura(dia_base, mes_base, ano_base, (uint8_t)(p * 30), &dia_v, &mes_v, &ano_v);

        for (int i = 0; i < MAX_PENDENCIAS; i++) {
            if (lista_pendencias[i].status == 0) {
                lista_pendencias[i].status    = 1;
                lista_pendencias[i].bandeira  = bandeira;
                strcpy(lista_pendencias[i].cartao, cartao);
                strcpy(lista_pendencias[i].valor, str_parcela);
                lista_pendencias[i].dia_venc  = dia_v;
                lista_pendencias[i].mes_venc  = mes_v;
                lista_pendencias[i].ano_venc  = ano_v;
                lista_pendencias[i].tentou_12h = 0;
                lista_pendencias[i].tentou_18h = 0;
                lista_pendencias[i].tentou_22h = 0;
                break;
            }
        }
    }
}

// =============================================================================
// ENTRADA DE DADOS
// =============================================================================

// congela a maquina esperando uma tecla. monitora * por 4 segundos para desligar.
char espera_tecla() {
    while (1) {
        if (tecla_pressionada_bruta('*')) {
            uint16_t tempo = 0;
            uint8_t falhas = 0;

            while (tempo < 400) { // 400 x 10ms = 4 segundos
                if (tecla_pressionada_bruta('*')) falhas = 0;
                else {
                    falhas++;
                    if (falhas > 5) break; // soltou por mais de 50ms: aborta cronometro
                }
                tempo++;
                _delay_ms(10);
            }

            if (tempo >= 400) {
                flag_desligar = 1; // sinaliza desligamento para o chamador tratar
                return 'X';
            } else {
                while (tecla_pressionada_bruta('*')) _delay_ms(10);
                return '*';
            }
        } else {
            char t = le_tecla();
            if (t != 0 && t != '*') return t;
        }
    }
}

// captura multiplos digitos e salva no buffer.
// retorna 1 se confirmado, 0 se cancelado ou desligamento solicitado.
// se o retorno for 0 e flag_desligar==1, o chamador deve desligar o sistema.
uint8_t le_dados_cliente(char* buffer, uint8_t tamanho_maximo, uint8_t ocultar, uint8_t auto_confirma) {
    uint8_t indice = 0;
    buffer[0] = '\0';

    while (1) {
        char tecla = espera_tecla();

        if (tecla == 'X') { flag_desligar = 1; return 0; } // desligamento urgente
        if (tecla == '*') return 0;                         // cancelamento normal
        if (tecla == '#' && indice > 0) return 1;           // confirmacao com enter

        if (tecla >= '0' && tecla <= '9' && indice < tamanho_maximo) {
            buffer[indice] = tecla;
            indice++;
            buffer[indice] = '\0';

            if (ocultar) display_dado('*');
            else         display_dado(tecla);

            if (auto_confirma && indice == tamanho_maximo) {
                _delay_ms(300);
                return 1;
            }
        }
    }
}

// =============================================================================
// COMUNICACAO COM SERVIDOR
// =============================================================================

// notifica o servidor sobre o login de um operador e recupera o nome cadastrado.
// se o servidor nao responder, usa nome padrao e acende o led vermelho.
void fazer_login_servidor(char id, char* nome_saida) {
    // nome padrao caso o servidor nao responda
    if      (id == '9') strcpy(nome_saida, "ADMIN");
    else if (id == '0') strcpy(nome_saida, "OPERADOR 0");
    else                strcpy(nome_saida, "OPERADOR 1");

    for (uint8_t tentativa = 0; tentativa < 3; tentativa++) {
        serial_limpa_buffer();
        serial_envia_char('M');
        serial_envia_char('L');
        serial_envia_char(id);

        char r1 = serial_recebe_char(3000);
        if (r1 != 'S') continue;

        PORTD &= ~(1 << PD3); // servidor respondeu: apaga led vermelho

        char r2 = serial_recebe_char(1000);
        if (r2 != 'L') continue;
        (void)serial_recebe_char(1000); // byte de tamanho (n), descarta

        uint8_t idx = 0;
        while (idx < 15) {
            char c = serial_recebe_char(1000);
            if (c == 0 || c == '\0') break;
            if (c != '"') nome_saida[idx++] = c;
        }
        nome_saida[idx] = '\0';
        return; // sucesso: sai sem acender led
    }

    // todas as tentativas falharam
    PORTD |= (1 << PD3); // acende led vermelho: servidor inalcancavel
}

// monta o pacote de dados e envia ao servidor. tenta 3 vezes com 40 segundos cada
// (120 segundos total). se esgotar sem resposta, exibe ERRO DE CONEXAO e acende o led vermelho.
char envia_transacao_e_espera(char tipo, char bandeira, char* cartao, char* senha, char* parcelas, char* valor) {
    char pacote_dados[40];
    pacote_dados[0] = '\0';

    char b_str[2] = {bandeira, '\0'};
    strcat(pacote_dados, b_str);
    strcat(pacote_dados, cartao);
    if (senha   != NULL) strcat(pacote_dados, senha);
    if (parcelas != NULL) strcat(pacote_dados, parcelas);
    strcat(pacote_dados, valor);

    uint8_t n = strlen(pacote_dados) + 1;

    for (uint8_t tentativa = 0; tentativa < 3; tentativa++) {
        serial_limpa_buffer();

        serial_envia_char('M');
        serial_envia_char(tipo);
        serial_envia_char(n);
        for (uint8_t i = 0; i < n; i++) serial_envia_char(pacote_dados[i]);

        if (tentativa > 0) {
            // avisa que esta tentando novamente antes de aguardar os proximos 40s
            display_limpar();
            display_string("TENTANDO DNV...");
        }

        // aguarda ate 40 segundos por resposta nesta tentativa
        char r1 = serial_recebe_char(40000);
        if (r1 != 'S') continue;

        PORTD &= ~(1 << PD3); // servidor respondeu: apaga led vermelho

        char r2 = serial_recebe_char(1000);
        if (r2 == 'X') return 'X';
        if (r2 != tipo) continue;

        char r3 = serial_recebe_char(1000);
        if (r3 != 0) return r3;
    }

    // esgotou 120 segundos sem resposta do servidor
    display_limpar();
    display_string("ERRO DE CONEXAO");
    PORTD |= (1 << PD3); // acende led vermelho
    _delay_ms(2500);
    return 0;
}

// envia o pedido de confirmacao de pagamento (comando MA) para uma parcela especifica.
// funcao interna usada apenas por processar_pendencias_servidor.
static char enviar_confirmacao_parcela(uint8_t idx) {
    char pacote_dados[20];
    pacote_dados[0] = '\0';
    char b_str[2] = {lista_pendencias[idx].bandeira, '\0'};
    strcat(pacote_dados, b_str);
    strcat(pacote_dados, lista_pendencias[idx].cartao);
    strcat(pacote_dados, lista_pendencias[idx].valor);
    uint8_t n = strlen(pacote_dados) + 1;

    for (uint8_t tentativa = 0; tentativa < 3; tentativa++) {
        serial_limpa_buffer();
        serial_envia_char('M');
        serial_envia_char('A');
        serial_envia_char(n);
        for (uint8_t j = 0; j < n; j++) serial_envia_char(pacote_dados[j]);

        char r1 = serial_recebe_char(5000);
        if (r1 != 'S') continue;

        PORTD &= ~(1 << PD3); // comunicacao estabelecida: apaga led vermelho

        char r2 = serial_recebe_char(1000);
        if (r2 != 'A') continue;
        char r3 = serial_recebe_char(1000);
        if (r3 != 0) return r3; // 'P' = pago, 'C' = cartao invalido, 'N' = nao encontrado
    }

    // todas as tentativas falharam: servidor inalcancavel
    PORTD |= (1 << PD3);
    return 0;
}

// =============================================================================
// FLUXOS DE VENDA
// =============================================================================

void fluxo_venda_vista() {
    char buffer_valor[10], buffer_bandeira[2], buffer_cartao[10], buffer_senha[10];

    display_limpar(); display_string("SELECIONADO:"); display_posiciona(1, 0); display_string("DEBITO"); _delay_ms(2000);
    display_limpar(); display_string("VALOR VENDA:"); display_posiciona(1, 0); display_string("R$ ");
    if (!le_dados_cliente(buffer_valor, 6, 0, 0)) return;

    int32_t valor_venda = atol(buffer_valor);

    display_limpar(); display_string("BANDEIRA (0-9):"); display_posiciona(1, 0);
    if (!le_dados_cliente(buffer_bandeira, 1, 0, 1)) return;
    char bandeira = buffer_bandeira[0];

    display_limpar(); display_string("NUMERO CARTAO:"); display_posiciona(1, 0);
    if (!le_dados_cliente(buffer_cartao, 6, 0, 0)) return;

    uint8_t tentativas = 0;
    while (1) {
        display_limpar(); display_string("SENHA CARTAO:"); display_posiciona(1, 0);
        if (!le_dados_cliente(buffer_senha, 6, 1, 0)) return;
        display_limpar(); display_string("PROCESSANDO...");

        if (bandeira == '0') { // processamento no banco interno da maquina
            uint8_t enc = 0;
            for (int i = 0; i < 5; i++) {
                if (strcmp(banco_local[i].numero, buffer_cartao) == 0) {
                    enc = 1;

                    if (banco_local[i].saldo_centavos >= valor_venda) {
                        banco_local[i].saldo_centavos -= valor_venda; // debita o valor

                        // verifica cashback apos a venda ser concluida
                        if (valor_venda >= 5000) { // compra acima de R$50,00
                            banco_local[i].compras_acima_50++;

                            if (banco_local[i].compras_acima_50 >= 3) {
                                // terceira compra qualificavel: credita R$20,00 no saldo
                                banco_local[i].saldo_centavos += 2000;
                                banco_local[i].compras_acima_50 = 0;
                                display_limpar(); display_string("CASHBACK R$20!"); _delay_ms(1500);
                            }
                        }

                        display_limpar(); display_string("LOCAL APROVADO"); _delay_ms(2000);
                    } else {
                        display_limpar(); display_string("SALDO INSUFIC."); _delay_ms(2000);
                    }
                    break;
                }
            }
            if (!enc) { display_limpar(); display_string("CARTAO INVALIDO"); _delay_ms(2000); }
            break;

        } else { // bandeira externa: comunica com servidor
            char res = envia_transacao_e_espera('V', bandeira, buffer_cartao, buffer_senha, NULL, buffer_valor);
            if (res == 'S') {
                display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000);
                tentativas++;
                if (tentativas >= 3) return;
            } else {
                display_limpar();
                if      (res == 'V') display_string("APROVADO");
                else if (res == 'C') display_string("CARTAO INVALIDO");
                else if (res == 'I') display_string("SALDO INSUFIC.");
                else if (res == 'X') display_string("RECUSADO");
                else                 display_string("ERRO CONEXAO");
                _delay_ms(2500);
                break;
            }
        }
    }
}

void fluxo_venda_parcelada() {
    char buffer_valor[10], buffer_bandeira[2], buffer_cartao[10], buffer_senha[10], buffer_parcelas[2];

    display_limpar(); display_string("SELECIONADO:"); display_posiciona(1, 0); display_string("CREDITO"); _delay_ms(2000);
    display_limpar(); display_string("VALOR VENDA:"); display_posiciona(1, 0); display_string("R$ ");
    if (!le_dados_cliente(buffer_valor, 6, 0, 0)) return;

    display_limpar(); display_string("QTD PARCELAS:"); display_posiciona(1, 0);
    if (!le_dados_cliente(buffer_parcelas, 1, 0, 0)) return;
    if (buffer_parcelas[0] < '1' || buffer_parcelas[0] > '3') {
        display_limpar(); display_string("MAX 3 PARCELAS!"); _delay_ms(2000); return;
    }

    display_limpar(); display_string("BANDEIRA (0-9):"); display_posiciona(1, 0);
    if (!le_dados_cliente(buffer_bandeira, 1, 0, 1)) return;
    char bandeira = buffer_bandeira[0];

    display_limpar(); display_string("NUMERO CARTAO:"); display_posiciona(1, 0);
    if (!le_dados_cliente(buffer_cartao, 6, 0, 0)) return;

    uint8_t tentativas = 0;
    while (1) {
        display_limpar(); display_string("SENHA CARTAO:"); display_posiciona(1, 0);
        if (!le_dados_cliente(buffer_senha, 6, 1, 0)) return;
        display_limpar(); display_string("PROCESSANDO...");

        if (bandeira == '0') {
            display_limpar(); display_string("SO A VISTA LOCAL"); _delay_ms(2000);
            break;
        } else {
            char res = envia_transacao_e_espera('P', bandeira, buffer_cartao, buffer_senha, buffer_parcelas, buffer_valor);
            if (res == 'S') {
                display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000);
                tentativas++;
                if (tentativas >= 3) return;
            } else {
                display_limpar();
                if (res == 'V') {
                    display_string("APROVADO");

                    // a primeira parcela foi cobrada pelo servidor agora.
                    // cria pendencias para as demais parcelas com as datas de vencimento corretas.
                    uint8_t num_parcelas = buffer_parcelas[0] - '0';
                    if (num_parcelas > 1) {
                        adicionar_pendencias_parceladas(bandeira, buffer_cartao, buffer_valor, num_parcelas);
                    }

                } else if (res == 'C') display_string("CARTAO INVALIDO");
                else if (res == 'I') display_string("SALDO INSUFIC.");
                else if (res == 'X') display_string("RECUSADO");
                else                 display_string("ERRO CONEXAO");
                _delay_ms(2500);
                break;
            }
        }
    }
}

void fluxo_estorno() {
    char buffer_dados[10], buffer_valor[10], buffer_bandeira[2], buffer_cartao[10];

    uint8_t tentativas = 0;
    while (1) {
        display_limpar(); display_string("SENHA OPERADOR:"); display_posiciona(1, 0);
        if (!le_dados_cliente(buffer_dados, 4, 1, 1)) return;
        if (strcmp(buffer_dados, "1254") == 0 ||
            strcmp(buffer_dados, "2349") == 0 ||
            strcmp(buffer_dados, "0738") == 0) break;
        else {
            display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000);
            tentativas++;
            if (tentativas >= 3) return;
        }
    }

    display_limpar(); display_string("VALOR ESTORNO:"); display_posiciona(1, 0); display_string("R$ ");
    if (!le_dados_cliente(buffer_valor, 6, 0, 0)) return;

    display_limpar(); display_string("BANDEIRA (0-9):"); display_posiciona(1, 0);
    if (!le_dados_cliente(buffer_bandeira, 1, 0, 1)) return;
    char bandeira = buffer_bandeira[0];

    display_limpar(); display_string("NUMERO CARTAO:"); display_posiciona(1, 0);
    if (!le_dados_cliente(buffer_cartao, 6, 0, 0)) return;

    display_limpar(); display_string("MODO: ESTORNO"); display_posiciona(1, 0); display_string("#-CONFIRMAR");
    char conf = 0;
    while (conf != '#') {
        conf = espera_tecla();
        if (conf == '*' || conf == 'X') return;
    }

    display_limpar(); display_string("PROCESSANDO...");
    if (bandeira == '0') {
        display_limpar(); display_string("ESTORNO LOCAL OK"); _delay_ms(2000);
    } else {
        char res = envia_transacao_e_espera('E', bandeira, buffer_cartao, NULL, NULL, buffer_valor);
        display_limpar();
        if      (res == 'V') display_string("ESTORNO EXT OK");
        else if (res == 'C') display_string("CARTAO INVALIDO");
        else if (res == 'X') display_string("RECUSADO PELO PC");
        else                 display_string("ERRO CONEXAO");
        _delay_ms(2500);
    }
}

// =============================================================================
// RELATORIOS E PENDENCIAS (acesso pelo administrador)
// =============================================================================

void relatorio_saldos_locais() {
    for (int i = 0; i < 5; i++) {
        char msg_saldo[16];
        int32_t reais     = banco_local[i].saldo_centavos / 100;
        int32_t centavos  = banco_local[i].saldo_centavos % 100;
        sprintf(msg_saldo, "R$ %ld,%02ld", reais, centavos);

        display_limpar();
        display_string("CARTAO ");
        display_string(banco_local[i].numero);
        display_posiciona(1, 0);
        display_string(msg_saldo);

        char op = 0;
        while (op == 0) {
            op = espera_tecla();
            if (op == 'X') return; // flag_desligar ja foi setada em espera_tecla
        }
    }
    display_limpar();
    display_string("FIM RELATORIO");
    _delay_ms(1500);
}

// exibe para o administrador todas as parcelas futuras registradas,
// mostrando cartao, data de vencimento e status de cada uma.
void exibir_pendencias_admin() {
    PORTD &= ~(1 << PD2); // apaga led amarelo: admin foi verificar

    uint8_t tem_pendencia = 0;
    for (int i = 0; i < MAX_PENDENCIAS; i++) {
        if (lista_pendencias[i].status != 0) {
            tem_pendencia = 1;

            // linha 0: numero do cartao
            display_limpar();
            display_string("CART:");
            display_string(lista_pendencias[i].cartao); // "CART:111111" = 11 chars

            // linha 1: data de vencimento e status
            display_posiciona(1, 0);

            // monta string da data manualmente (DD/MM/AA)
            char data_str[9];
            data_str[0] = (lista_pendencias[i].dia_venc / 10) + '0';
            data_str[1] = (lista_pendencias[i].dia_venc % 10) + '0';
            data_str[2] = '/';
            data_str[3] = (lista_pendencias[i].mes_venc / 10) + '0';
            data_str[4] = (lista_pendencias[i].mes_venc % 10) + '0';
            data_str[5] = '/';
            data_str[6] = (lista_pendencias[i].ano_venc / 10) + '0';
            data_str[7] = (lista_pendencias[i].ano_venc % 10) + '0';
            data_str[8] = '\0';

            display_string(data_str); // "15/06/26" = 8 chars
            display_string(" ");

            if      (lista_pendencias[i].status == 1) display_string("PEND");   // aguardando
            else if (lista_pendencias[i].status == 2) display_string("INAD!");  // inadimplente

            // aguarda toque para ver o proximo registro
            char op = 0;
            while (op == 0) {
                op = espera_tecla();
                if (op == 'X') return; // flag_desligar ja setada em espera_tecla
            }
        }
    }

    display_limpar();
    if (!tem_pendencia) display_string("NENHUMA PENDENTE");
    else                display_string("FIM LISTA");
    _delay_ms(2000);
}

// verifica no servidor se as parcelas cujo vencimento e hoje ja foram pagas.
// hora_slot indica qual janela horaria disparou a checagem (12, 18 ou 22).
//
// logica de slots:
// - 12h: tenta apenas o slot das 12h se ainda nao foi tentado
// - 18h: tenta 12h (se pendente) e 18h
// - 22h: tenta 12h, 18h e 22h (todos os que ainda nao foram tentados)
//        e so declara inadimplencia depois que os tres slots falharem
//
// isso garante que mesmo com ajuste manual do relogio para 21:59, os tres
// testes serao executados antes de qualquer decisao de inadimplencia.
void processar_pendencias_servidor(uint8_t hora_slot) {
    uint8_t dia_hoje, mes_hoje, ano_hoje;
    leitura_data_numerica(&dia_hoje, &mes_hoje, &ano_hoje);

    uint8_t falhou_alguma = 0;

    for (int i = 0; i < MAX_PENDENCIAS; i++) {
        if (lista_pendencias[i].status != 1) continue; // apenas aguardando

        // pula registros com vencimento em outra data
        if (lista_pendencias[i].dia_venc != dia_hoje ||
            lista_pendencias[i].mes_venc != mes_hoje ||
            lista_pendencias[i].ano_venc != ano_hoje) continue;

        uint8_t pago = 0;

        // slot 12h: tenta sempre que ainda nao foi tentado, independente do hora_slot.
        // isso garante cobertura mesmo quando o relogio foi ajustado manualmente para apos as 12h.
        if (!pago && lista_pendencias[i].tentou_12h == 0) {
            char r = enviar_confirmacao_parcela(i);
            lista_pendencias[i].tentou_12h = (r == 'P') ? 1 : 2;
            if (r == 'P') pago = 1;
        }

        // slot 18h: tenta se estamos no slot 18h ou no 22h (para cobrir o ajuste manual)
        if (!pago && hora_slot >= 18 && lista_pendencias[i].tentou_18h == 0) {
            char r = enviar_confirmacao_parcela(i);
            lista_pendencias[i].tentou_18h = (r == 'P') ? 1 : 2;
            if (r == 'P') pago = 1;
        }

        // slot 22h: tenta somente quando o hora_slot for 22
        if (!pago && hora_slot == 22 && lista_pendencias[i].tentou_22h == 0) {
            char r = enviar_confirmacao_parcela(i);
            lista_pendencias[i].tentou_22h = (r == 'P') ? 1 : 2;
            if (r == 'P') pago = 1;
        }

        if (pago) {
            lista_pendencias[i].status = 0; // parcela paga: libera a gaveta
        } else if (hora_slot == 22 &&
                   lista_pendencias[i].tentou_12h == 2 &&
                   lista_pendencias[i].tentou_18h == 2 &&
                   lista_pendencias[i].tentou_22h == 2) {
            // todos os tres slots do dia falharam: cartao inadimplente
            lista_pendencias[i].status = 2;
            falhou_alguma = 1;
        }
    }

    if (falhou_alguma) PORTD |= (1 << PD2); // acende led amarelo: ha inadimplentes
}
