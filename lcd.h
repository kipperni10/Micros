#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>

void inicializa_display(void);
void display_comando(uint8_t comando);
void display_dado(uint8_t dado);
void display_string(const char *texto);
void display_limpar(void);
void display_posiciona(uint8_t linha, uint8_t coluna);
void display_on(void);
void display_off(void);

#endif /* DISPLAY_H_ */
