// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: transacao

#define F_CPU 16000000UL  // 16MHz, Unsigned (somente positivos) Long (32 bits)
#include <avr/io.h> // portas
#include <util/delay.h> // pausas e atrasos
#include <stdint.h>
#include <string.h> // textos
#include <stdlib.h>
#include <stdio.h>
#include "display.h"
#include "teclado.h"
#include "transacao.h"
#include "serial.h"
#include "relogio.h"

// banco local com cashback
typedef struct {
	char numero[7];
	int32_t saldo_centavos; // saldo em centavos para evitar uso de float que trava o processador
	uint8_t compras_acima_50; // contador de cashback na memoria
} CartaoLocal;

// simula um banco de dados na memoria ram para testes offline
CartaoLocal banco_local[5] = {
	{"111111", 80000, 0},
	{"222222", 80000, 0},
	{"333333", 80000, 0},
	{"444444", 80000, 0},
	{"555555", 80000, 0}
};

// memoria de pendências
#define MAX_PENDENCIAS 10
typedef struct {
	uint8_t status; // 0 = livre, 1 = aguardando envio, 2 = falha permanente
	char bandeira;
	char cartao[7];
	char valor[10];
} Pendencia;

Pendencia lista_pendencias[MAX_PENDENCIAS]; // cria 10 gavetas na memoria ram

// varre as 10 gavetas ate achar uma vazia (status 0) e salva os dados da transacao offline
void adiciona_pendencia(char bandeira, char* cartao, char* valor) {
	for(int i = 0; i < MAX_PENDENCIAS; i++) {
		if (lista_pendencias[i].status == 0) {
			lista_pendencias[i].status = 1;
			lista_pendencias[i].bandeira = bandeira;
			strcpy(lista_pendencias[i].cartao, cartao);
			strcpy(lista_pendencias[i].valor, valor);
			break; // salva e sai do laco para nao duplicar
		}
	}
}

// congela a maquina esperando o operador digitar, mas monitora 3 segundos no * para desligar
char espera_tecla() {
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
			if (tempo >= 300) return 'X'; // forca desligamento
			else {
				while(tecla_pressionada_bruta('*')) _delay_ms(10);
				return '*'; // cancela operacao atual
			}
		}
		else {
			char t = le_tecla();
			if (t != 0 && t != '*') return t;
		}
	}
}

// captura multiplos digitos e salva no array buffer
uint8_t le_dados_cliente(char* buffer, uint8_t tamanho_maximo, uint8_t ocultar, uint8_t auto_confirma) {
	uint8_t indice = 0;
	buffer[0] = '\0'; // inicializa o texto vazio
	while(1) {
		char tecla = espera_tecla();
		if (tecla == 'X' || tecla == '*') return 0; // abortou
		if (tecla == '#' && indice > 0) return 1; // confirmou com enter
		
		if (tecla >= '0' && tecla <= '9' && indice < tamanho_maximo) {
			buffer[indice] = tecla; // guarda na gaveta atual
			indice++; // aponta para a proxima
			buffer[indice] = '\0'; // fecha o texto
			
			if (ocultar) display_dado('*'); // mascara de senha
			else display_dado(tecla);
			
			// se bateu o limite e auto_confirma for 1, encerra sozinho sem precisar do #
			if (auto_confirma && indice == tamanho_maximo) {
				_delay_ms(300);
				return 1;
			}
		}
	}
}

// login
void fazer_login_servidor(char id, char* nome_saida) {
	if (id == '9') strcpy(nome_saida, "ADMIN");
	else if (id == '0') strcpy(nome_saida, "OPERADOR 0");
	else strcpy(nome_saida, "OPERADOR 1");
	
	serial_limpa_buffer(); // vassoura na porta rx
	serial_envia_char('M'); // pacote padrao de inicio
	serial_envia_char('L'); // comando de login
	serial_envia_char(id); // quem esta logando
	
	char r1 = serial_recebe_char(3000); // espera ate 3s pelo pc
	if (r1 == 'S') {
		PORTD &= ~(1 << PD3); // se o servidor respondeu apaga o led imediatamente
		char r2 = serial_recebe_char(1000);
		if (r2 == 'L') {
			(void)serial_recebe_char(1000);
			uint8_t idx = 0;
			while(idx < 15) { // le os caracteres do nome enviado
				char c = serial_recebe_char(1000);
				if (c == 0 || c == '\0') break; // fim do texto
				if (c != '"') {
					nome_saida[idx++] = c;
				}
			}
			nome_saida[idx] = '\0';
		}
	}
}

// constroi o pacote de dados longo e faz 3 tentativas de envio pela serial
char envia_transacao_e_espera(char tipo, char bandeira, char* cartao, char* senha, char* parcelas, char* valor) {
	char pacote_dados[40];
	pacote_dados[0] = '\0';
	
	// monta o super texto juntando as pecas (strcat adiciona no final)
	char b_str[2] = {bandeira, '\0'};
	strcat(pacote_dados, b_str);
	strcat(pacote_dados, cartao);
	if (senha != NULL) strcat(pacote_dados, senha);
	if (parcelas != NULL) strcat(pacote_dados, parcelas);
	strcat(pacote_dados, valor);
	
	uint8_t n = strlen(pacote_dados) + 1; // conta o tamanho total do pacote
	
	for (uint8_t tentativa = 0; tentativa < 3; tentativa++) { // laco de 3 tentativas em caso de ruidos
		serial_limpa_buffer();
		
		serial_envia_char('M');
		serial_envia_char(tipo);
		serial_envia_char(n);
		for (uint8_t i = 0; i < n; i++) {
			serial_envia_char(pacote_dados[i]); // envia letra por letra
		}
		
		char r1 = serial_recebe_char(5000);
		if (r1 != 'S') { // se o servidor nao devolver S de sucesso
			if (tentativa < 2) {
				display_limpar();
				display_string("TENTANDO DNV..."); // tenta enviar de novo
				_delay_ms(1000);
			}
			continue;
		}
		
		PORTD &= ~(1 << PD3); // se o servidor respondeu apaga o led imediatamente

		char r2 = serial_recebe_char(1000);
		if (r2 == 'X') return 'X'; // pc mandou forcar logoff
		if (r2 != tipo) continue;
		
		char r3 = serial_recebe_char(1000);
		if (r3 != 0) return r3; // retorna o codigo de aprovacao/reprovacao final
	}
	return 0; // falha apos 3 tentativas
}

void relatorio_saldos_locais() {
	for(int i = 0; i < 5; i++) { // varre os 5 cartoes cadastrados na ram
		char msg_saldo[16];
		int32_t reais = banco_local[i].saldo_centavos / 100; // pega a parte inteira
		int32_t centavos = banco_local[i].saldo_centavos % 100; // pega o resto (os centavos reais)
		sprintf(msg_saldo, "R$ %ld,%02ld", reais, centavos); // formata visualmente como dinheiro

		display_limpar();
		display_string("CARTAO ");
		display_string(banco_local[i].numero);
		display_posiciona(1, 0);
		display_string(msg_saldo);
		
		char op = 0;
		while (op == 0) { // trava a tela ate o admin apertar algum botao para ver o proximo
			op = espera_tecla();
			if (op == 'X') return; // forca saida
		}
	}
	display_limpar();
	display_string("FIM RELATORIO");
	_delay_ms(1500);
}

void fluxo_venda_vista() {
	char buffer_valor[10], buffer_bandeira[2], buffer_cartao[10], buffer_senha[10];
	
	// blocos sucessivos de interface visual e coleta de dados (se retornar 0, aborta o fluxo inteiro)
	display_limpar(); display_string("SELECIONADO:"); display_posiciona(1, 0); display_string("DEBITO"); _delay_ms(2000);
	display_limpar(); display_string("VALOR VENDA:"); display_posiciona(1, 0); display_string("R$ ");
	if (!le_dados_cliente(buffer_valor, 6, 0, 0)) return;
	
	int32_t valor_venda = atol(buffer_valor); // converte string de valor para numero inteiro long
	
	display_limpar(); display_string("BANDEIRA (0-9):"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_bandeira, 1, 0, 1)) return;
	char bandeira = buffer_bandeira[0];
	
	display_limpar(); display_string("NUMERO CARTAO:"); display_posiciona(1, 0);
	if (!le_dados_cliente(buffer_cartao, 6, 0, 0)) return;
	
	while(1) {
		display_limpar(); display_string("SENHA CLIENTE:"); display_posiciona(1, 0);
		if (!le_dados_cliente(buffer_senha, 6, 1, 0)) return;
		display_limpar(); display_string("PROCESSANDO...");
		
		if (bandeira == '0') { // 0 = processamento no banco interno da propria maquina
			uint8_t enc = 0;
			for(int i = 0; i < 5; i++) { // varre os 5 cartoes locais
				if (strcmp(banco_local[i].numero, buffer_cartao) == 0) {
					enc = 1; // encontrou o cartao
					
					if (banco_local[i].compras_acima_50 >= 3) { // logica do cashback
						display_limpar(); display_string("CASHBACK R$20!"); _delay_ms(1500);
						if (valor_venda <= 2000) valor_venda = 0; // se a compra for menor que 20, zera
						else valor_venda -= 2000; // senao desconta 20 reais (2000 centavos)
						banco_local[i].compras_acima_50 = 0; // zera contador
					}
					
					if (banco_local[i].saldo_centavos >= valor_venda) { // verifica fundos
						banco_local[i].saldo_centavos -= valor_venda; // debita da memoria
						if (valor_venda >= 5000) banco_local[i].compras_acima_50++; // soma contador para compras >= R$50
						display_limpar(); display_string("LOCAL APROVADO"); _delay_ms(2000);
						} else {
						display_limpar(); display_string("SALDO INSUFIC."); _delay_ms(2000);
					}
					break;
				}
			}
			if (!enc) { display_limpar(); display_string("CARTAO INVALIDO"); _delay_ms(2000); }
			break;
			} else { // se a bandeira nao for 0, manda pacote pelo cabo serial para o computador
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
	if (buffer_parcelas[0] < '1' || buffer_parcelas[0] > '3') { // trava logica imposta
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
		
		if (bandeira == '0') { // banco interno so roda no debito
			display_limpar(); display_string("SO A VISTA LOCAL"); _delay_ms(2000);
			break;
			} else {
			// envia passando o buffer_parcelas preenchido
			char res = envia_transacao_e_espera('P', bandeira, buffer_cartao, buffer_senha, buffer_parcelas, buffer_valor);
			if (res == 'S') {
				display_limpar(); display_string("SENHA INVALIDA"); _delay_ms(2000);
				} else {
				display_limpar();
				if (res == 'V') {
					display_string("EXTERNO APROVADO");
					adiciona_pendencia(bandeira, buffer_cartao, buffer_valor); // credito gera pendencia offline para fechamento futuro
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
	
	while(1) { // protecao extra: exige senha do operador ou admin para estornar dinheiro
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
	while(conf != '#') { // barreira final de seguranca para evitar estorno acidental
		conf = espera_tecla();
		if (conf == '*' || conf == 'X') return;
	}
	
	display_limpar(); display_string("PROCESSANDO...");
	if (bandeira == '0') { display_limpar(); display_string("ESTORNO LOCAL OK"); _delay_ms(2000); }
	else { // envia pro servidor com NULL na senha e nas parcelas
		char res = envia_transacao_e_espera('E', bandeira, buffer_cartao, NULL, NULL, buffer_valor);
		display_limpar();
		if (res == 'V') display_string("ESTORNO EXT OK");
		else if (res == 'C') display_string("CARTAO INVALIDO");
		else if (res == 'X') display_string("RECUSADO PELO PC");
		else display_string("ERRO CONEXAO");
		_delay_ms(2500);
	}
}

// exibe na tela para o administrador todas as transacoes aguardando upload
void exibir_pendencias_admin() {
	PORTD &= ~(1 << PD3); // desliga o led de alerta vermelho no pino d3

	uint8_t tem_pendencia = 0;
	for(int i = 0; i < MAX_PENDENCIAS; i++) {
		if(lista_pendencias[i].status != 0) { // se a gaveta nao estiver vazia
			tem_pendencia = 1;
			display_limpar();
			display_string("CARTAO: "); display_string(lista_pendencias[i].cartao);
			display_posiciona(1, 0);
			
			if (lista_pendencias[i].status == 1) display_string("ST: AGUARDANDO");
			else if (lista_pendencias[i].status == 2) display_string("ST: FALHOU!");
			
			char op = 0;
			while (op == 0) { // aguarda toque para pular para a proxima
				op = espera_tecla();
				if (op == 'X') return;
			}
		}
	}
	display_limpar();
	if(!tem_pendencia) display_string("NENHUMA PENDENTE");
	else display_string("FIM LISTA");
	_delay_ms(2000);
}

// rotina automatica chamada no menu para tentar subir as vendas offline pro computador
void processar_pendencias_servidor() {
	uint8_t falhou_alguma = 0;
	for(int i = 0; i < MAX_PENDENCIAS; i++) { // varre o array inteiro
		if (lista_pendencias[i].status == 1) { // se achar uma gaveta 'aguardando'
			char pacote_dados[40];
			pacote_dados[0] = '\0';
			char b_str[2] = {lista_pendencias[i].bandeira, '\0'};
			strcat(pacote_dados, b_str);
			strcat(pacote_dados, lista_pendencias[i].cartao);
			strcat(pacote_dados, lista_pendencias[i].valor);
			
			uint8_t n = strlen(pacote_dados) + 1;
			char res_final = 0;
			
			for (uint8_t tentativa = 0; tentativa < 3; tentativa++) { // 3 tentativas
				serial_limpa_buffer();
				serial_envia_char('M');
				serial_envia_char('A'); // comando especial 'A' para liquidar pendencia
				serial_envia_char(n);
				for (uint8_t j = 0; j < n; j++) serial_envia_char(pacote_dados[j]);
				
				char r1 = serial_recebe_char(5000);
				if (r1 != 'S') continue;
				
				PORTD &= ~(1 << PD3); // se o servidor respondeu apaga o led imediatamente

				char r2 = serial_recebe_char(1000);
				if (r2 != 'A') continue;
				char r3 = serial_recebe_char(1000);
				if (r3 != 0) {
					res_final = r3;
					break;
				}
			}
			if (res_final == 'P') lista_pendencias[i].status = 0; // sucesso: libera a gaveta da ram
			else if (res_final == 'C' || res_final == 'N') {
				lista_pendencias[i].status = 2; // falha: marca gaveta como permanente e acende led
				falhou_alguma = 1;
			}
		}
	}
	if (falhou_alguma) PORTD |= (1 << PD3); // acende o led no pino d3 alertando o adm
}