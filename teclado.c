// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: teclado

#define F_CPU 16000000UL // 16MHz, Unsigned (somente positivos) Long (32 bits)
#include <avr/io.h> // portas
#include <util/delay.h> // pausas e atrasos
#include "teclado.h"

void configura_pinos_teclado() {
	// D4 a D7 como saidas, 5V
	DDRD |= 0xF0;
	PORTD |= 0xF0;
	
	// C0 a C2 como entradas, liga os resistores de pull-up internos e forca 5V quando soltos
	DDRC &= ~0x07;
	PORTC |= 0x07;
}

// funcao de debounce
uint8_t debounce(uint8_t pino_coluna) {
	uint8_t count = 0;
	uint8_t key_now = 0, key_last = 0;
	
	while (count < 7) { // conta ate 7 para confirmar estabilizacao
		_delay_ms(2);
		
		if (pino_coluna == 0) key_now = (PINC & (1 << PC0)) ? 1 : 0; // foca em um pino, zera todos os outros e transforma em 1 se tiver 5V ou 0 se tiver 0v
		else if (pino_coluna == 1) key_now = (PINC & (1 << PC1)) ? 1 : 0;
		else if (pino_coluna == 2) key_now = (PINC & (1 << PC2)) ? 1 : 0;
		
		if (key_now == key_last) count++; // incrementa
		else { count = 0; key_last = key_now; }
	}
	
	return key_now;
}

char le_tecla() {
	const char keys[4][3] = {
		{'1','2','3'},
		{'4','5','6'},
		{'7','8','9'},
		{'*','0','#'}
	};
	
	for (int r = 0; r < 4; r++) { // linhas
		PORTD |= 0xF0; PORTD &= ~(1 << (r + 4)); _delay_us(10); // joga 5V em todas as linhas e escolhe apenas a linha atual do teste e joga 0 (+4 por iniciar em D4)
		
		for (int c = 0; c < 3; c++) { // colunas
			if (!(PINC & (1 << c))) { // o botao pressionado = 0V, encontrado o pressionado
				if (debounce(c) == 0) {
					while (debounce(c) == 0);
					return keys[r][c];
				}
			}
		}
	}
	
	return 0; // caso nenhum botao esteja pressionado
}


// teclas fora da digitacao normal
uint8_t tecla_pressionada_bruta(char tecla_alvo) {
	if (tecla_alvo == '#') { // linha 4, coluna 3
		PORTD |= 0xF0; PORTD &= ~(1 << PD7); _delay_us(10); // joga 0V na linha 4
		if (!(PINC & (1 << PC2))) return 1; //checa a coluna 3, se for 0V o # esta pressionado, retorna 1 verdadeiro
	}
	
	if (tecla_alvo == '*') { // linha 4, coluna 1
		PORTD |= 0xF0; PORTD &= ~(1 << PD7); _delay_us(10); // joga 0V na linha 4
		if (!(PINC & (1 << PC0))) return 1; //checa a coluna 1, se for 0V o * esta pressionado, retorna 1 verdadeiro
	}
	
	return 0; // caso nenhum esteja pressionado
}