// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: relogio

#ifndef RELOGIO_H_
#define RELOGIO_H_

#include <stdint.h>

void iniciar_relogio(void);
void ajustar_relogio(uint8_t h, uint8_t m, uint8_t s);
void leitura_horas(char* buffer);
void reset_tempo_comunicacao(void);

#endif