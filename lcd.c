#include "lcd.h"

// Frequência do oscilador (Necessário para a função __delay_ms)
// !!!!! dtem ajustar isso no main.h ou aqui se for diferente de 20MHz
#define _XTAL_FREQ 20000000 

void lcd_cmd(char cmd) {
    LCD_DATA = cmd;
    LCD_RS = 0; // 0 para Comando
    LCD_EN = 1;
    __delay_ms(2);
    LCD_EN = 0;
}

void lcd_char(char data) {
    LCD_DATA = data;
    LCD_RS = 1; // 1 para Dados (Caracteres)
    LCD_EN = 1;
    __delay_ms(2);
    LCD_EN = 0;
}

void lcd_init(void) {
    __delay_ms(20);
    lcd_cmd(0x38); // Configura display para 8 bits, 2 linhas, fonte 5x7
    lcd_cmd(0x0C); // Liga display, desliga cursor
    lcd_cmd(0x06); // Incrementa o cursor para a direita
    lcd_cmd(0x01); // Limpa o display
    __delay_ms(2);
}

void lcd_string(const char *str) {
    while(*str != '\0') {
        lcd_char(*str);
        str++;
    }
}

void lcd_clear(void) {
    lcd_cmd(0x01);
    __delay_ms(2);
}

void lcd_set_cursor(char row, char col) {
    char pos;
    if(row == 1) {
        pos = 0x80 + col; // Linha 1 começa no endereço 0x80
    } else if(row == 2) {
        pos = 0xC0 + col; // Linha 2 começa no endereço 0xC0
    }
    lcd_cmd(pos);
}