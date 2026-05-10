// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: admin_menu

#define F_CPU 16000000UL // 16MHz, Unsigned (somente positivos) Long (32 bits)
#include <avr/io.h> // portas
#include <util/delay.h> // pausas e atrasos
#include <stdint.h>
#include <string.h>
#include "display.h"
#include "teclado.h"
#include "transacao.h"
#include "relogio.h"
#include "admin_menu.h"

// estado padrao: 1 = ativo, 0 = bloqueado
uint8_t op0_ativo = 1; 
uint8_t op1_ativo = 1;
uint8_t pendencias_ativas = 1;

uint8_t operador_ativo(uint8_t id_operador) {
    if (id_operador == 0) return op0_ativo; // id 0 para operador 0
    if (id_operador == 1) return op1_ativo; // id 1 para o operador 1
    return 0;
}

void menu_config_operadores() {
    display_limpar(); 
    display_string("1-OP0: "); 
    
    // checa o status do operador 0 e imprime ON ou OFF
    if (op0_ativo == 1) {
        display_string("ON");
    } else {
        display_string("OFF");
    }
    
    display_posiciona(1, 0); // cursor para a linha de baixo, primeira coluna
    display_string("2-OP1: "); 
    
    // checa o status do operador 1 e imprime ON ou OFF
    if (op1_ativo == 1) {
        display_string("ON");
    } else {
        display_string("OFF");
    }
    
    char op = espera_tecla(); // trava a tela esperando o adm digitar
    
    // ! como interruptor, 0 vira 1 e 1 vira 0
    if (op == '1') op0_ativo = !op0_ativo;
    if (op == '2') op1_ativo = !op1_ativo;
    
    if (op == '1' || op == '2') { // opcoes validas
        display_limpar(); display_string("SALVO!"); _delay_ms(1000); // mensagem de confirmacao na tela por 1seg
    }
}

void menu_config_hora() {
    char buffer[5]; // 4 digitos + \0
    display_limpar();
    display_string("NOVA HORA(HHMM):");
    display_posiciona(1, 0); // cursor para a linha de baixo, primeira coluna
    
    if (le_dados_cliente(buffer, 4, 0, 1)) { // se a leitura das 4 teclas deu certo
        // converte texto ascii em numero real, multiplica a dezena por 10 e soma a unidade
        uint8_t h = (buffer[0]-'0')*10 + (buffer[1]-'0');
        uint8_t m = (buffer[2]-'0')*10 + (buffer[3]-'0');
        
        if (h < 24 && m < 60) { // trava de seguranca logica
            ajustar_relogio(h, m, 0); // atualiza o hardware do relogio e zera os segundos
            display_limpar(); display_string("HORA SALVA!");
        } else {
            display_limpar(); display_string("HORA INVALIDA!");
        }
        _delay_ms(1500); // 1.5 seg com a mensagem na tela
    }
}

void menu_config_pendencias() {
    display_limpar();
    display_string("1-VER PENDENCIAS");
    display_posiciona(1, 0);
    display_string("2-MODO: "); 
    
    // checa o status do modo de pendencias e imprime ON ou OFF
    if (pendencias_ativas == 1) {
        display_string("ON"); // caso esteja aceitando pendencias
    } else {
        display_string("OFF"); // caso nao esteja aceitando pendencias
    }
    
    char op = espera_tecla();
    if (op == '2') { // 2 = ligar ou desligar o modo de pendencias
        pendencias_ativas = !pendencias_ativas; // 1 vira 0 e 0 vira 1
        display_limpar(); display_string("SALVO!"); _delay_ms(1000); // salva a escolha por 1seg
    } else if (op == '1') { // leitura das pendencias acumuladas
        exibir_pendencias_admin();
    }
}

char fluxo_menu_admin() {
    while(1) { // loop infinito para o menu nao fechar sozinho apos cada configuracao
        display_limpar();
        display_string("1-OP 2-HR"); // linha 0, resumido para caber o relogio no canto direito
        display_posiciona(1, 0);
        display_string("3-PND 4-SALDO"); // linha 1
        
        char op = 0; // gaveta vazia
        uint8_t tempo_refresh = 0; // contador de voltas

        while(op == 0) { // roda continuamente enquanto nenhuma tecla for apertada
            tempo_refresh++; // soma uma volta
            if (tempo_refresh >= 10) { // a cada 10 voltas (200ms no total), atualiza o relogio na tela
                tempo_refresh = 0; // zera o contador para o proximo ciclo
                char hora_atual[6]; // espaco para HH:MM e \0
                leitura_horas(hora_atual); // busca a hora no hardware
                display_posiciona(0, 11); // escreve o HH:MM bem no canto superior direito
                display_string(hora_atual); // carimba a hora nova
            }

            if (tecla_pressionada_bruta('*')) { 
                uint16_t tempo = 0;
                uint8_t falhas = 0; 
                
                while (tempo < 300) { // roda ate dar 3 segundos
                    if (tecla_pressionada_bruta('*')) {
                        falhas = 0; // se continua pressionado, zera as falhas
                    } else {
                        falhas++; // se soltou rapidamente, soma uma falha
                        if (falhas > 5) break; // se soltou por mais de 50ms, assume que tirou o dedo e aborta
                    }
                    tempo++;
                    _delay_ms(10);
                }
                
                if (tempo >= 300) op = 'X'; // se segurou os 3s inteiros, atribui X para desligar a maquina
                else {
                    while(tecla_pressionada_bruta('*')) _delay_ms(10); // se foi toque curto, trava ate o botao subir
                    op = '*'; // atribui * para apenas fazer logoff e voltar ao menu de login
                }
            }
            else { // se nao era a tecla *, faz a leitura normal
                char t = le_tecla();
                if (t != 0 && t != '*') op = t; // se alguma tecla util foi digitada, guarda na gaveta op
            }
            
            if (op == 0) _delay_ms(20); // alivio para o processador nao rodar o while na velocidade maxima
        }
        
        // executa a acao baseada na tecla pressionada
        if (op == '*') return '*'; // logoff
        if (op == 'X') return 'X'; // desligar
        
        if (op == '1') menu_config_operadores(); // configuracao de operadores
        if (op == '2') menu_config_hora(); // configuracao de hora
        if (op == '3') menu_config_pendencias(); // configuracao de pendencias
        if (op == '4') relatorio_saldos_locais(); // relatorio de saldo
    }
}