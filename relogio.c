#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "relogio.h"

// Variáveis voláteis porque são alteradas dentro de uma interrupção
volatile uint8_t r_horas = 0;
volatile uint8_t r_minutos = 0;
volatile uint8_t r_segundos = 0;

void relogio_init() {
	// Configura Timer1 para Modo CTC (Clear Timer on Compare Match)
	// Prescaler de 256. (16MHz / 256 = 62500 ticks por segundo)
	TCCR1B |= (1 << WGM12) | (1 << CS12);
	
	// Define o topo da contagem para 1 segundo (62500 - 1)
	OCR1A = 62499;
	
	// Habilita a interrupção do Timer1
	TIMSK1 |= (1 << OCIE1A);
	
	// Habilita interrupções globais do microcontrolador
	sei();
}

// Essa função roda sozinha a cada 1 segundo
ISR(TIMER1_COMPA_vect) {
	r_segundos++;
	if (r_segundos >= 60) {
		r_segundos = 0;
		r_minutos++;
		if (r_minutos >= 60) {
			r_minutos = 0;
			r_horas++;
			if (r_horas >= 24) r_horas = 0;
		}
	}
}

void relogio_set(uint8_t h, uint8_t m, uint8_t s) {
	cli(); // Pausa interrupções para não dar conflito ao alterar
	r_horas = h;
	r_minutos = m;
	r_segundos = s;
	sei(); // Retoma interrupções
}

// Preenche um texto no formato "HH:MM"
void relogio_get_string(char* buffer) {
	cli();
	uint8_t h = r_horas, m = r_minutos;
	sei();
	
	buffer[0] = (h / 10) + '0';
	buffer[1] = (h % 10) + '0';
	buffer[2] = ':';
	buffer[3] = (m / 10) + '0';
	buffer[4] = (m % 10) + '0';
	buffer[5] = '\0';
}