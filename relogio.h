#ifndef RELOGIO_H_
#define RELOGIO_H_

#include <stdint.h>

void relogio_init(void);
void relogio_set(uint8_t h, uint8_t m, uint8_t s);
void relogio_get_string(char* buffer);

#endif /* RELOGIO_H_ */