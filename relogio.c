// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: relogio

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "relogio.h"

volatile uint8_t r_horas   = 0;
volatile uint8_t r_minutos = 0;
volatile uint8_t r_segundos = 0;
volatile uint8_t r_dia = 1;
volatile uint8_t r_mes = 1;
volatile uint8_t r_ano = 26; // dois digitos: 26 = 2026

// flag_checar_pendencias agora armazena o horario que disparou (0 = sem pendencia, 12 / 18 / 22 = horario)
extern uint8_t flag_checar_pendencias;

void iniciar_relogio() {
    TCCR1B |= (1 << WGM12) | (1 << CS12); // CTC, prescaler 256: 16000000/256 = 62500 ticks/s
    OCR1A = 62499;                          // dispara a cada 1 segundo exato
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

            if (r_horas >= 24) {
                r_horas = 0;

                // avanca o dia ao virar da meia-noite
                uint8_t max_dias;
                if (r_mes == 2) {
                    max_dias = (r_ano % 4 == 0) ? 29 : 28;
                } else if (r_mes == 4 || r_mes == 6 || r_mes == 9 || r_mes == 11) {
                    max_dias = 30;
                } else {
                    max_dias = 31;
                }

                r_dia++;
                if (r_dia > max_dias) {
                    r_dia = 1;
                    r_mes++;
                    if (r_mes > 12) {
                        r_mes = 1;
                        r_ano++;
                        if (r_ano > 99) r_ano = 0;
                    }
                }
            }
        }
    }

    // levanta a flag nos horarios de verificacao de parcelas pendentes
    // armazena qual horario disparou para o processamento saber qual slot usar
    if (r_minutos == 0 && r_segundos == 0) {
        if      (r_horas == 12) flag_checar_pendencias = 12;
        else if (r_horas == 18) flag_checar_pendencias = 18;
        else if (r_horas == 22) flag_checar_pendencias = 22;
    }
}

void ajustar_relogio(uint8_t h, uint8_t m, uint8_t s) {
    cli();
    r_horas    = h;
    r_minutos  = m;
    r_segundos = s;
    sei();
}

void ajustar_data(uint8_t dia, uint8_t mes, uint8_t ano) {
    cli();
    r_dia = dia;
    r_mes = mes;
    r_ano = ano;
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

void leitura_data(char* buffer) {
    cli();
    uint8_t d = r_dia, m = r_mes, a = r_ano;
    sei();

    buffer[0] = (d / 10) + '0';
    buffer[1] = (d % 10) + '0';
    buffer[2] = '/';
    buffer[3] = (m / 10) + '0';
    buffer[4] = (m % 10) + '0';
    buffer[5] = '/';
    buffer[6] = (a / 10) + '0';
    buffer[7] = (a % 10) + '0';
    buffer[8] = '\0';
}

void leitura_data_numerica(uint8_t* dia, uint8_t* mes, uint8_t* ano) {
    cli();
    *dia = r_dia;
    *mes = r_mes;
    *ano = r_ano;
    sei();
}

// calcula a data resultante de adicionar N dias a partir de uma data base
void calcular_data_futura(uint8_t dia, uint8_t mes, uint8_t ano, uint8_t dias,
                           uint8_t* dia_out, uint8_t* mes_out, uint8_t* ano_out) {
    uint8_t dias_por_mes[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (ano % 4 == 0) dias_por_mes[1] = 29;

    uint8_t d = dia, m = mes, a = ano;
    for (uint8_t i = 0; i < dias; i++) {
        d++;
        if (d > dias_por_mes[m - 1]) {
            d = 1;
            m++;
            if (m > 12) {
                m = 1;
                a++;
                if (a > 99) a = 0;
            }
        }
    }

    *dia_out = d;
    *mes_out = m;
    *ano_out = a;
}
