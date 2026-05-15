// Projeto 1 - MicPay - Bianca Bitencourt, Henrique Bitencourt e Nicolas da Silveira Kipper //

// Arquivo: relogio

#ifndef RELOGIO_H_
#define RELOGIO_H_

#include <stdint.h>

void iniciar_relogio(void);
void ajustar_relogio(uint8_t h, uint8_t m, uint8_t s);
void ajustar_data(uint8_t dia, uint8_t mes, uint8_t ano);
void leitura_horas(char* buffer);
void leitura_data(char* buffer);
void leitura_data_numerica(uint8_t* dia, uint8_t* mes, uint8_t* ano);
void calcular_data_futura(uint8_t dia, uint8_t mes, uint8_t ano, uint8_t dias,
                           uint8_t* dia_out, uint8_t* mes_out, uint8_t* ano_out);

#endif
