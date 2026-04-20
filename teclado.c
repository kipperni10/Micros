#include "teclado.h"

#define _XTAL_FREQ 20000000

char ler_teclado(void) {
    // Varredura da Linha 1
    ROW1 = 0; ROW2 = 1; ROW3 = 1; ROW4 = 1;
    if(COL1 == 0) { __delay_ms(20); while(COL1==0); return '1'; }
    if(COL2 == 0) { __delay_ms(20); while(COL2==0); return '2'; }
    if(COL3 == 0) { __delay_ms(20); while(COL3==0); return '3'; }
    if(COL4 == 0) { __delay_ms(20); while(COL4==0); return 'A'; }
    
    // Varredura da Linha 2
    ROW1 = 1; ROW2 = 0; ROW3 = 1; ROW4 = 1;
    if(COL1 == 0) { __delay_ms(20); while(COL1==0); return '4'; }
    if(COL2 == 0) { __delay_ms(20); while(COL2==0); return '5'; }
    if(COL3 == 0) { __delay_ms(20); while(COL3==0); return '6'; }
    if(COL4 == 0) { __delay_ms(20); while(COL4==0); return 'B'; }
    
    // Varredura da Linha 3
    ROW1 = 1; ROW2 = 1; ROW3 = 0; ROW4 = 1;
    if(COL1 == 0) { __delay_ms(20); while(COL1==0); return '7'; }
    if(COL2 == 0) { __delay_ms(20); while(COL2==0); return '8'; }
    if(COL3 == 0) { __delay_ms(20); while(COL3==0); return '9'; }
    if(COL4 == 0) { __delay_ms(20); while(COL4==0); return 'C'; }
    
    // Varredura da Linha 4
    ROW1 = 1; ROW2 = 1; ROW3 = 1; ROW4 = 0;
    if(COL1 == 0) { __delay_ms(20); while(COL1==0); return '*'; } // Botão CANCELA
    if(COL2 == 0) { __delay_ms(20); while(COL2==0); return '0'; }
    if(COL3 == 0) { __delay_ms(20); while(COL3==0); return '#'; } // Botão CONFIRMA
    if(COL4 == 0) { __delay_ms(20); while(COL4==0); return 'D'; }
    
    return 0; // Retorna 0 se nenhuma tecla foi pressionada
}