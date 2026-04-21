#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "teclado.h"

void configura_pinos_teclado() {
	DDRD |= 0xF0;  PORTD |= 0xF0;
	DDRC &= ~0x07; PORTC |= 0x07;
}

uint8_t debounce(uint8_t pino_coluna) {
	uint8_t count = 0;
	uint8_t key_now, key_last = 0;
	while (count < 7) {
		_delay_ms(2);
		if (pino_coluna == 0) key_now = (PINC & (1 << PC0)) ? 1 : 0;
		else if (pino_coluna == 1) key_now = (PINC & (1 << PC1)) ? 1 : 0;
		else if (pino_coluna == 2) key_now = (PINC & (1 << PC2)) ? 1 : 0;
		if (key_now == key_last) count++;
		else { count = 0; key_last = key_now; }
	}
	return key_now;
}

char le_tecla() {
	const char keys[4][3] = {{'1','2','3'},{'4','5','6'},{'7','8','9'},{'*','0','#'}};
	for (int r = 0; r < 4; r++) {
		PORTD |= 0xF0; PORTD &= ~(1 << (r + 4)); _delay_us(10);
		for (int c = 0; c < 3; c++) {
			if (!(PINC & (1 << c))) {
				if (debounce(c) == 0) {
					while (debounce(c) == 0);
					return keys[r][c];
				}
			}
		}
	}
	return 0;
}

uint8_t tecla_pressionada_bruta(char tecla_alvo) {
	if (tecla_alvo == '#') {
		PORTD |= 0xF0; PORTD &= ~(1 << PD7); _delay_us(10);
		if (!(PINC & (1 << PC2))) return 1;
	}
	if (tecla_alvo == '*') {
		PORTD |= 0xF0; PORTD &= ~(1 << PD7); _delay_us(10);
		if (!(PINC & (1 << PC0))) return 1;
	}
	return 0;
}
