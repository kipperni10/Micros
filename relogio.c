#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "relogio.h"

volatile uint8_t r_horas = 0;
volatile uint8_t r_minutos = 0;
volatile uint8_t r_segundos = 0;
volatile uint8_t tempo_sem_comunicacao = 0;
volatile uint8_t flag_checar_pendencias = 0; // Inicia abaixada

void iniciar_relogio() {
	TCCR1B |= (1 << WGM12) | (1 << CS12);
	OCR1A = 62499;
	TIMSK1 |= (1 << OCIE1A);
	sei();
}

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
	
	// --- GATILHO DA FASE 3: HORÁRIOS DE VERIFICAÇÃO ---
	if (r_segundos == 0 && r_minutos == 0) {
		if (r_horas == 12 || r_horas == 18 || r_horas == 22) {
			flag_checar_pendencias = 1; // Levanta a bandeira para o sistema checar!
		}
	}
	
	// --- Lógica do LED Vermelho (Fora do Ar) ---
	if (tempo_sem_comunicacao < 120) {
		tempo_sem_comunicacao++;
	}
	
	if (tempo_sem_comunicacao >= 120) {
		PORTD |= (1 << PD2);
	}
}

void ajustar_relogio(uint8_t h, uint8_t m, uint8_t s) {
	cli();
	r_horas = h;
	r_minutos = m;
	r_segundos = s;
	sei();
}

void leitura_horas(char* buffer) {
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

void reset_tempo_comunicacao() {
	cli();
	tempo_sem_comunicacao = 0;
	PORTD &= ~(1 << PD2);
	sei();
}