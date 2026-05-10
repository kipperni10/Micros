// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: serial

#define F_CPU 16000000UL // 16MHz, Unsigned (somente positivos) Long (32 bits)
#include <avr/io.h> // portas
#include <util/delay.h> // pausas e atrasos
#include "serial.h"

void conf_comunicacao() {
    // configuracao da velocidade, 19200 bps
    // UBRR = (16000000 / (16 * 19200)) - 1 = 51.08 (arredondado para 51)
    UBRR0H = (unsigned char)(51 >> 8); // desloca 8 casas para guardar a parte alta
    UBRR0L = (unsigned char)51; // 51 (00110011) fica alocado totalmente na parte baixa
    
    UCSR0B = (1 << TXEN0) | (1 << RXEN0); // habilita transmissor TX e receptor RX
    
    // define o formato do pacote: paridade par, 1 stop bit (0) e tamanho de 8 bits de dados (3)
    UCSR0C = (1 << UPM01) | (0 << UPM00) | (0 << USBS0) | (3 << UCSZ00);
}

void serial_envia_char(char c) {
    while (!(UCSR0A & (1 << UDRE0))); // 1 = pronto para uso, 0 = ocupado enviando a letra anterior
    UDR0 = c; // se 1, coloca novo caractere
}

char serial_recebe_char(uint16_t timeout_ms) { // escuta a serial por um tempo e retorna 0 se nao receber nada
    uint16_t tempo = 0;
    while (!(UCSR0A & (1 << RXC0))) { // fica aguardando o dado chegar
        _delay_ms(1);
        tempo++;
        if (tempo >= timeout_ms) return 0; // tempo esgotado
    }
    return UDR0; // retorna o caractere recebido
}

// limpa caracteres fantasmas da porta antes de iniciar uma nova leitura
void serial_limpa_buffer(void) {
    while (UCSR0A & (1 << RXC0)) { // enquanto tiver alguma coisa presa na gaveta de recepcao
        volatile char lixo = UDR0; // le o lixo para forcar o hardware a esvaziar (volatile impede o compilador de apagar a linha)
    }
}