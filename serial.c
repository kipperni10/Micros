#define F_CPU 16000000UL
#include <avr/io.h>
#include "serial.h"

void serial_init() {
	// Configura o Baud Rate para 19200 usando clock de 16MHz
	// UBRR = (16000000 / (16 * 19200)) - 1 = 51.08 -> 51
	UBRR0H = (unsigned char)(51 >> 8);
	UBRR0L = (unsigned char)51;
	
	// Habilita apenas o Transmissor (TX) por enquanto
	UCSR0B = (1 << TXEN0);
	
	// Configura o Frame: 8 bits de dados, 1 stop bit, Paridade Par (Even)
	// UPM01=1, UPM00=0 (Paridade Par)
	// USBS0=0 (1 Stop bit)
	// UCSZ01=1, UCSZ00=1 (8 Data bits)
	UCSR0C = (1 << UPM01) | (0 << UPM00) | (0 << USBS0) | (3 << UCSZ00);
}

void serial_envia_char(char c) {
	// Espera o buffer de hardware esvaziar
	while (!(UCSR0A & (1 << UDRE0)));
	// Coloca o novo caractere no pino de saída
	UDR0 = c;
}