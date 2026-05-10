// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: relogio

#define F_CPU 16000000UL // 16MHz, Unsigned (somente positivos) Long (32 bits)
#include <avr/io.h> // portas
#include <avr/interrupt.h>
#include "relogio.h"

// volatile pois e variavel e precisa sempre ser reconsultada
volatile uint8_t r_horas = 0;
volatile uint8_t r_minutos = 0;
volatile uint8_t r_segundos = 0;

void iniciar_relogio() {
    TCCR1B |= (1 << WGM12) | (1 << CS12); // CS12 = prescaler de 256, 16.000.000/256 = 62500
    
    OCR1A = 62499; // Output Compare Register, em qual numero o alarme toca (62500-1)
    
    TIMSK1 |= (1 << OCIE1A); // quando chega na contagem acusa, e a mascara de interrupcoes
    
    sei(); // habilita as interrupcoes globais
}

ISR(TIMER1_COMPA_vect) { // TIMER1_COMPA_vect, faz rodar o laco toda vez que o timer aciona, a cada 1seg
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

void ajustar_relogio(uint8_t h, uint8_t m, uint8_t s) { // uint8_t para inteiro, positivo e 8 bits
    cli(); // desabilito as interrupcoes, mesmo o timer de 1seg acionando o relogio congela
    
    // insiro manualmente
    r_horas = h;
    r_minutos = m; 
    r_segundos = s;
    
    sei(); // habilito as interrupcoes
}

void leitura_horas(char* buffer) {
    cli();
    uint8_t h = r_horas, m = r_minutos; // HH:MM
    sei();
    
    buffer[0] = (h / 10) + '0'; // isolo a dezena
    buffer[1] = (h % 10) + '0'; // isolo a unidade
    buffer[2] = ':';
    buffer[3] = (m / 10) + '0'; // isolo a dezena
    buffer[4] = (m % 10) + '0'; // isolo a unidade
    buffer[5] = '\0';
}