#ifndef TECLADO_H
#define	TECLADO_H

#include <xc.h>

// Mapeamento de linhas (Saídas) e Colunas (Entradas)
// !!!! precisa ajustar os TRIS (direção) e PORT/LAT no main.
#define ROW1 LATCbits.LATC0
#define ROW2 LATCbits.LATC1
#define ROW3 LATCbits.LATC2
#define ROW4 LATCbits.LATC3

#define COL1 PORTCbits.RC4
#define COL2 PORTCbits.RC5
#define COL3 PORTCbits.RC6
#define COL4 PORTCbits.RC7

char ler_teclado(void);

#endif	/* TECLADO_H */