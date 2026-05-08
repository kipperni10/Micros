#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

void serial_init(void);
void serial_envia_char(char c);
char serial_recebe_char_timeout(uint16_t timeout_ms);
void serial_limpa_buffer(void); // <-- Nova função declarada aqui

#endif /* SERIAL_H_ */