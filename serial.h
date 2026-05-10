// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: serial

#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

void conf_comunicacao(void);
void serial_envia_char(char c);
char serial_recebe_char(uint16_t timeout_ms);
void serial_limpa_buffer(void);

#endif 