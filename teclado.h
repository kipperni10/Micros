#ifndef TECLADO_H_
#define TECLADO_H_

#include <stdint.h>

void configura_pinos_teclado(void);
uint8_t debounce(uint8_t pino_coluna);
char le_tecla(void);
uint8_t tecla_pressionada_bruta(char tecla_alvo);

#endif /* TECLADO_H_ */
