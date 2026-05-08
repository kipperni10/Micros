#ifndef TRANSACAO_H_
#define TRANSACAO_H_

#include <stdint.h>

char espera_tecla_com_shutdown(void);
uint8_t le_dados_cliente(char* buffer, uint8_t tamanho_maximo, uint8_t ocultar, uint8_t auto_submit);
void fluxo_venda_vista(void);
void fluxo_venda_parcelada(void);
void fluxo_estorno(void);
void relatorio_saldos_locais(void);
void exibir_pendencias_admin(void);
void processar_pendencias_servidor(void);
void fazer_login_servidor(char id, char* nome_saida); // <-- Nova função de Login

#endif