#ifndef SA_API_H
#define SA_API_H

#include <stdio.h>     /* SEEK_SET, SEEK_CUR, SEEK_END */
#include <stdint.h>
#include <stdbool.h>

/* Arquivo aberto: qual inodo e em que posição está o "cursor". */
typedef struct {
    uint8_t  inodo;
    uint16_t posicao;
    bool     em_uso;
} ARQUIVO;

void inicializa_SA(void);
void formata_SA(void);

/* Modos: "w" (cria ou apaga o conteúdo) e "r+" ("r" é tratado igual a "r+").
 * Retorna NULL se o arquivo não existe (r+) ou se não há inodo livre (w). */
ARQUIVO *meu_fopen(const char *nome, const char *modo);

/* Retornam quantos bytes foram de fato lidos/escritos. */
uint16_t meu_fread(ARQUIVO *a, uint8_t *buf, uint16_t tam);
uint16_t meu_fwrite(ARQUIVO *a, const uint8_t *buf, uint16_t tam);

/* Retorna 0, ou -1 se a nova posição ficaria fora de [0, tamanho]. */
int meu_fseek(ARQUIVO *a, int32_t desloc, int origem);

uint16_t meu_ftell(ARQUIVO *a);
bool meu_feof(ARQUIVO *a);
void meu_fclose(ARQUIVO *a);

/* Apaga o arquivo e devolve seus blocos. Retorna 0, ou -1 se não existe. */
int meu_remove(const char *nome);

/* Imprime nome e tamanho de todos os arquivos. */
void lista_todos(void);

#endif
