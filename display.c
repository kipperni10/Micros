// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: display

#define F_CPU 16000000UL // 16MHz, Unsigned (somente positivos) Long (32 bits)
#include <avr/io.h>      // portas
#include <util/delay.h>  // pausas e atrasos
#include "display.h"

void pulso() {
    PORTB |= (1 << PB1);
    _delay_us(1);         // 5V por 1microseg

    PORTB &= ~(1 << PB1);
    _delay_us(100);       // 0V por 0,1miliseg
}

void envia_bits(uint8_t valor, uint8_t tipo) { // uint8_t para inteiro, positivo e 8 bits
    if (tipo) {
        PORTB |= (1 << PB0); // é um dado, algo a ser mostrado
    } else {
        PORTB &= ~(1 << PB0); // é um comando, limpar etc
    }

    PORTB &= ~((1 << PB2) | (1 << PB3) | (1 << PB4)); // inicia limpando
    PORTC &= ~(1 << PC3); // inicia limpando

    if (valor & 0x01) PORTB |= (1 << PB2); // locação bit 1
    if (valor & 0x02) PORTB |= (1 << PB3); // locação bit 2
    if (valor & 0x04) PORTB |= (1 << PB4); // locação bit 3
    if (valor & 0x08) PORTC |= (1 << PC3); // locação bit 4

    pulso();
}

void display_comando(uint8_t comando) { // uint8_t para inteiro, positivo e 8 bits
    envia_bits(comando >> 4, 0);   // envia os 4 bits superiores
    envia_bits(comando & 0x0F, 0); // envia os 4 bits inferiores
    _delay_ms(2);
}

void display_dado(uint8_t dado) {
    envia_bits(dado >> 4, 1);   // envia os 4 bits superiores
    envia_bits(dado & 0x0F, 1); // envia os 4 bits inferiores
    _delay_us(50);
}

void display_string(const char *texto) {
    while (*texto) {
        display_dado(*texto++); // digitador automático
    }
}

void display_limpar() {
    display_comando(0x01); // limpa todo o lcd e joga o cursor para cima e esquerda
    _delay_ms(2);
}

void display_posiciona(uint8_t linha, uint8_t coluna) {
    uint8_t endereco;

    if (linha == 0) {
        endereco = 0x80 + coluna; // se for a linha de cima
    } else {
        endereco = 0xC0 + coluna; // se for a linha de baixo
    }

    display_comando(endereco);
}

void display_on() {
    display_comando(0x0C); // texto ligado
    PORTC |= (1 << PC4);   // luz de fundo ligada
}

void display_off() {
    display_comando(0x08); // texto desligado
    PORTC &= ~(1 << PC4);  // luz de fundo desligada
}

void inicializa_display() {
    DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB4); // configura as portas como saídas
    DDRC |= (1 << PC3) | (1 << PC4);                                        // configura as portas como saídas
    _delay_ms(50);

    // envia 3 vezes, dica do manual
    envia_bits(0x03, 0);
    _delay_ms(5);        // 1° pausa longa

    envia_bits(0x03, 0);
    _delay_us(150);      // 2° pausa curta

    envia_bits(0x03, 0); // a certeza
    envia_bits(0x02, 0); // entra em 4bits

    display_comando(0x28); // 2 linhas, fonte 5x8
    display_off();         // desliga temporariamente para o usuário não ver resíduos
    display_limpar();      // garantia tela 100% em branco
}