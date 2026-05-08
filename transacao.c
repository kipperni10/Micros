#define F_CPU 16000000UL // 16MHz
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

// --- BANCO LOCAL COM CASHBACK ---
typedef struct {
	char numero[7];
	int32_t saldo_centavos;
	uint8_t compras_acima_50; // Contador de Cashback
} CartaoLocal;

CartaoLocal banco_local[5] = {
	{"111111", 80000, 0},
	{"222222", 80000, 0},
	{"333333", 80000, 0},
	{"444444", 80000, 0},
	{"555555", 80000, 0}
};

// --- MEMÓRIA DE PENDÊNCIAS ---
#define MAX_PENDENCIAS 10
typedef struct {
	uint8_t status;
	char bandeira;
	char cartao[7];
	char valor[10];
} Pendencia;

Pendencia lista_pendencias[MAX_PENDENCIAS];

void adiciona_pendencia(char bandeira, char* cartao, char* valor) {
	for(int i = 0; i < MAX_PENDENCIAS; i++) {
		if (lista_pendencias[i].status == 0) {
			lista_pendencias[i].status = 1;
			lista_pendencias[i].bandeira = bandeira;
			strcpy(lista_pendencias[i].cartao, cartao);
			strcpy(lista_pendencias[i].valor, valor);
			break;
		}
	}
}

char espera_tecla_com_shutdown() {
	while(1) {
		if (tecla_pressionada_bruta('*')) {
			uint16_t tempo = 0;
			uint8_t falhas = 0;
			while (tempo < 300) {
				if (tecla_pressionada_bruta('*')) falhas = 0;
				else { falhas++; if (falhas > 5) break; }
				tempo++;
				_delay_ms(10);
			}
			if (tempo >= 300) return 'X';
			else {
				while(tecla_pressionada_bruta('*')) _delay_ms(10);
				return '*';
			}
		}
		else {
			char t = le_tecla();
			if (t != 0 && t != '*') return t;
		}
	}
}

uint8_t le_dados_cliente(char* buffer, uint8_t tamanho_maximo, uint8_t ocultar, uint8_t auto_submit) {
	uint8_t indice = 0;
	buffer[0] = '\0';
	while(1) {
		char tecla = espera_tecla_com_shutdown();
		if (tecla == 'X' || tecla == '*') return 0;
		if (tecla == '#' && indice > 0) return 1;
		
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

// --- LOGIN (Com correção do LED) ---
void fazer_login_servidor(char id, char* nome_saida) {
	if (id == '9') strcpy(nome_saida, "ADMIN");
	else if (id == '0') strcpy(nome_saida, "OPERADOR 0");
	else strcpy(nome_saida, "OPERADOR 1");
	
	serial_limpa_buffer();
	serial_envia_char('M');
	serial_envia_char('L');
	serial_envia_char(id);
	
	char r1 = serial_recebe_char_timeout(3000);
	if (r1 == 'S') {
		reset_tempo_comunicacao(); // O SERVIDOR RESPONDEU! APAGA O LED IMEDIATAMENTE!
		char r2 = serial_recebe_char_timeout(1000);
		if (r2 == 'L') {
			char r3 = serial_recebe_char_timeout(1000);
			uint8_t idx = 0;
			while(idx < 15) {
				char c = serial_recebe_char_timeout(1000);
				if (c == 0 || c == '\0') break;
				if (c != '"') {
					nome_saida[idx++] = c;
				}
			}
			nome_saida[idx] = '\0';
		}
	}
}

char envia_transacao_e_espera(char tipo, char bandeira, char* cartao, char* senha, char* parcelas, char* valor) {
	char payload[40];
	payload[0] = '\0';
	
	char b_str[2] = {bandeira, '\0'};
	strcat(payload, b_str);
	strcat(payload, cartao);
	if (senha != NULL) strcat(payload, senha);
	if (parcelas != NULL) strcat(payload, parcelas);
	strcat(payload, valor);
	
	uint8_t n = strlen(payload) + 1;
	
	for (uint8_t tentativa = 0; tentativa < 3; tentativa++) {
		serial_limpa_buffer();
		
		serial_envia_char('M');
		serial_envia_char(tipo);
		serial_envia_char(n);
		for (uint8_t i = 0; i < n; i++) {
			serial_envia_char(payload[i]);
		}
		
		char r1 = serial_recebe_char_timeout(5000);
		if (r1 != 'S') {
			if (tentativa < 2) {
				display_limpar();
				display_string("TENTANDO DNV...");
				_delay_ms(1000);
			}
			continue;
		}
		
		reset_tempo_comunicacao(); // O SERVIDOR RESPONDEU! APAGA O LED IMEDIATAMENTE!

		char r2 = serial_recebe_char_timeout(1000);
		if (r2 == 'X') return 'X';
		if (r2 != tipo) continue;
		
		char r3 = serial_recebe_char_timeout(1000);
		if (r3 != 0) return r3;
	}
	return 0;
}

void relatorio_saldos_locais() {
	for(int i = 0; i < 5; i++) {
		char msg_saldo[16];
		int32_t reais = banco_local[i].saldo_centavos / 100;
		int32_t centavos = banco_local[i].saldo_centavos % 100;
		sprintf(msg_saldo, "R$ %ld,%02ld", reais, centavos);

		display_limpar();
		display_string("CARTAO ");
		display_string(banco_local[i].numero);
		display_posiciona(1, 0);
		display_string(msg_saldo);
		
		char op = 0;
		while (op == 0) {
			op = espera_tecla_com_shutdown();
			if (op == 'X') return;
		}
	}
	display_limpar();
	display_string("FIM RELATORIO");
	_delay_ms(1500);
}

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
	while(1) {
		display_limpar(); display_string("SENHA CLIENTE:"); display_posiciona(1, 0);
		if (!le_dados_cliente(buffer_senha, 6, 1, 0)) return;
		display_limpar(); display_string("PROCESSANDO...");
		
		if (bandeira == '0') {
			uint8_t enc = 0;
			for(int i = 0; i < 5; i++) {
				if (strcmp(banco_local[i].numero, buffer_cartao) == 0) {
					enc = 1;
					
					if (banco_local[i].compras_acima_50 >= 3) {
						display_limpar(); display_string("CASHBACK R$20!"); _delay_ms(1500);
						if (valor_venda <= 2000) valor_venda = 0;
						else valor_venda -= 2000;
						banco_local[i].compras_acima_50 = 0;
					}
					
					if (banco_local[i].saldo_centavos >= valor_venda) {
						banco_local[i].saldo_centavos -= valor_venda;
						if (valor_venda >= 5000) banco_local[i].compras_acima_50++;
						display_limpar(); display_string("LOCAL APROVADO"); _delay_ms(2000);
						} else {
						display_limpar(); display_string("SALDO INSUFIC."); _delay_ms(2000);
					}
					break;
				}
			}
			if (!enc) { display_limpar(); display_string("CARTAO INVALIDO"); _delay_ms(2000); }
			break;
			} else {
			char res = envia_transacao_e_espera('V', bandeira, buffer_cartao, buffer_senha, NULL, buffer_valor);
			if (res == 'S') {
				display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000);
				} else {
				display_limpar();
				if (res == 'V') display_string("EXTERNO APROVADO");
				else if (res == 'C') display_string("CARTAO INVALIDO");
				else if (res == 'I') display_string("SALDO INSUFIC.");
				else if (res == 'X') display_string("RECUSADO PELO PC");
				else display_string("ERRO CONEXAO");
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
	while(1) {
		display_limpar(); display_string("SENHA CLIENTE:"); display_posiciona(1, 0);
		if (!le_dados_cliente(buffer_senha, 6, 1, 0)) return;
		display_limpar(); display_string("PROCESSANDO...");
		if (bandeira == '0') {
			display_limpar(); display_string("SO A VISTA LOCAL"); _delay_ms(2000);
			break;
			} else {
			char res = envia_transacao_e_espera('P', bandeira, buffer_cartao, buffer_senha, buffer_parcelas, buffer_valor);
			if (res == 'S') {
				display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000);
				} else {
				display_limpar();
				if (res == 'V') {
					display_string("EXTERNO APROVADO");
					adiciona_pendencia(bandeira, buffer_cartao, buffer_valor);
				}
				else if (res == 'C') display_string("CARTAO INVALIDO");
				else if (res == 'I') display_string("SALDO INSUFIC.");
				else if (res == 'X') display_string("RECUSADO PELO PC");
				else display_string("ERRO CONEXAO");
				_delay_ms(2500);
				break;
			}
		}
	}
}

void fluxo_estorno() {
	char buffer_dados[10], buffer_valor[10], buffer_bandeira[2], buffer_cartao[10];
	while(1) {
		display_limpar(); display_string("SENHA OPERADOR:"); display_posiciona(1, 0);
		if (!le_dados_cliente(buffer_dados, 4, 1, 1)) return;
		if (strcmp(buffer_dados, "1254") == 0 || strcmp(buffer_dados, "2349") == 0 || strcmp(buffer_dados, "0738") == 0) break;
		else { display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000); }
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
	while(conf != '#') {
		conf = espera_tecla_com_shutdown();
		if (conf == '*' || conf == 'X') return;
	}
	display_limpar(); display_string("PROCESSANDO...");
	if (bandeira == '0') { display_limpar(); display_string("ESTORNO LOCAL OK"); _delay_ms(2000); }
	else {
		char res = envia_transacao_e_espera('E', bandeira, buffer_cartao, NULL, NULL, buffer_valor);
		display_limpar();
		if (res == 'V') display_string("ESTORNO EXT OK");
		else if (res == 'C') display_string("CARTAO INVALIDO");
		else if (res == 'X') display_string("RECUSADO PELO PC");
		else display_string("ERRO CONEXAO");
		_delay_ms(2500);
	}
}

void exibir_pendencias_admin() {
	PORTD &= ~(1 << PD3);

	uint8_t tem_pendencia = 0;
	for(int i = 0; i < MAX_PENDENCIAS; i++) {
		if(lista_pendencias[i].status != 0) {
			tem_pendencia = 1;
			display_limpar();
			display_string("CARTAO: "); display_string(lista_pendencias[i].cartao);
			display_posiciona(1, 0);
			if (lista_pendencias[i].status == 1) display_string("ST: AGUARDANDO");
			else if (lista_pendencias[i].status == 2) display_string("ST: FALHOU!");
			char op = 0;
			while (op == 0) {
				op = espera_tecla_com_shutdown();
				if (op == 'X') return;
			}
		}
	}
	display_limpar();
	if(!tem_pendencia) display_string("NENHUMA PENDENTE");
	else display_string("FIM LISTA");
	_delay_ms(2000);
}

void processar_pendencias_servidor() {
	uint8_t falhou_alguma = 0;
	for(int i = 0; i < MAX_PENDENCIAS; i++) {
		if (lista_pendencias[i].status == 1) {
			char payload[40];
			payload[0] = '\0';
			char b_str[2] = {lista_pendencias[i].bandeira, '\0'};
			strcat(payload, b_str);
			strcat(payload, lista_pendencias[i].cartao);
			strcat(payload, lista_pendencias[i].valor);
			
			uint8_t n = strlen(payload) + 1;
			char res_final = 0;
			
			for (uint8_t tentativa = 0; tentativa < 3; tentativa++) {
				serial_limpa_buffer();
				serial_envia_char('M');
				serial_envia_char('A');
				serial_envia_char(n);
				for (uint8_t j = 0; j < n; j++) serial_envia_char(payload[j]);
				
				char r1 = serial_recebe_char_timeout(5000);
				if (r1 != 'S') continue;
				
				reset_tempo_comunicacao(); // O SERVIDOR RESPONDEU! APAGA O LED IMEDIATAMENTE!

				char r2 = serial_recebe_char_timeout(1000);
				if (r2 != 'A') continue;
				char r3 = serial_recebe_char_timeout(1000);
				if (r3 != 0) {
					res_final = r3;
					break;
				}
			}
			if (res_final == 'P') lista_pendencias[i].status = 0;
			else if (res_final == 'C' || res_final == 'N') {
				lista_pendencias[i].status = 2;
				falhou_alguma = 1;
			}
		}
	}
	if (falhou_alguma) PORTD |= (1 << PD3);
}