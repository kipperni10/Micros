#ifndef LCD_H
#define	LCD_H

#include <xc.h> // Biblioteca padrão do compilador XC8

// !!!! precisará mapear estes pinos no MPLABX de acordo com as ligações do PICsimLab ou Placa Física.
#define LCD_RS LATBbits.LATB0 // Exemplo: Pino B0
#define LCD_EN LATBbits.LATB1 // Exemplo: Pino B1
#define LCD_DATA LATD         // Exemplo: Porta D inteira para dados (8 bits)

void lcd_cmd(char cmd);
void lcd_init(void);
void lcd_char(char data);
void lcd_string(const char *str);
void lcd_clear(void);
void lcd_set_cursor(char row, char col);

#endif	/* LCD_H */