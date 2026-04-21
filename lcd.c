#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "display.h"

void display_pulse_enable() {
	PORTB |= (1 << PB1);  _delay_us(1);
	PORTB &= ~(1 << PB1); _delay_us(100);
}

void display_send_4bits(uint8_t valor, uint8_t tipo) {
	if (tipo) PORTB |= (1 << PB0); else PORTB &= ~(1 << PB0);
	PORTB &= ~((1 << PB2) | (1 << PB3) | (1 << PB4));
	PORTC &= ~(1 << PC3);
	if (valor & 0x01) PORTB |= (1 << PB2);
	if (valor & 0x02) PORTB |= (1 << PB3);
	if (valor & 0x04) PORTB |= (1 << PB4);
	if (valor & 0x08) PORTC |= (1 << PC3);
	display_pulse_enable();
}

void display_comando(uint8_t comando) {
	display_send_4bits(comando >> 4, 0);
	display_send_4bits(comando & 0x0F, 0);
	_delay_ms(2);
}

void display_dado(uint8_t dado) {
	display_send_4bits(dado >> 4, 1);
	display_send_4bits(dado & 0x0F, 1);
	_delay_us(50);
}

void display_string(const char *texto) {
	while (*texto) display_dado(*texto++);
}

void display_limpar() {
	display_comando(0x01);
	_delay_ms(2);
}

void display_posiciona(uint8_t linha, uint8_t coluna) {
	uint8_t endereco = (linha == 0) ? (0x80 + coluna) : (0xC0 + coluna);
	display_comando(endereco);
}

void display_on() {
	display_comando(0x0C);
	PORTC |= (1 << PC4);
}

void display_off() {
	display_comando(0x08);
	PORTC &= ~(1 << PC4);
}

void inicializa_display() {
	DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB4);
	DDRC |= (1 << PC3) | (1 << PC4);
	_delay_ms(50);
	display_send_4bits(0x03, 0); _delay_ms(5);
	display_send_4bits(0x03, 0); _delay_us(150);
	display_send_4bits(0x03, 0);
	display_send_4bits(0x02, 0);
	display_comando(0x28);
	display_off();
	display_limpar();
}
