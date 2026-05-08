#ifndef RELOGIO_H_
#define RELOGIO_H_

#include <stdint.h>

extern volatile uint8_t flag_checar_pendencias; // <-- Nova flag

void iniciar_relogio(void);
void ajustar_relogio(uint8_t h, uint8_t m, uint8_t s);
void leitura_horas(char* buffer);
void reset_tempo_comunicacao(void);

#endif