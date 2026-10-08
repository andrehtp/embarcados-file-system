/*
 * Camada de memória — versão Linux.
 * Emula a EEPROM AT24C512 com o arquivo PARTICAO.BIN (64 KiB).
 * Baseado no código do enunciado (seção 2.1).
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "memoria.h"

#define NOME_ARQUIVO "PARTICAO.BIN"

static FILE *f = NULL;   /* o arquivo que emula o chip */

/* Retorna 1 se o intervalo [posicao, posicao + tam) cabe na memória. */
static int dentro_da_memoria(uint16_t posicao, uint16_t tam)
{
    /* Soma em uint32_t: em uint16_t, 65535 + 2 daria 1 e passaria no teste. */
    uint32_t fim = (uint32_t)posicao + tam;
    return fim <= MEMORIA_TAMANHO;
}

/* Retorna o tamanho do arquivo aberto, em bytes. */
static long tamanho_do_arquivo(void)
{
    fseek(f, 0, SEEK_END);
    return ftell(f);
}

/* Grava MEMORIA_TAMANHO bytes 0xFF, como uma EEPROM nova de fábrica. */
static void preenche_com_ff(void)
{
    uint8_t pagina[128];
    memset(pagina, 0xFF, sizeof pagina);

    fseek(f, 0, SEEK_SET);
    for (unsigned int i = 0; i < MEMORIA_TAMANHO / sizeof pagina; i++)
    {
        fwrite(pagina, sizeof pagina, 1, f);
    }
    fflush(f);
}

void memoria_init(void)
{
    printf("Chamou a versao de mem.init do linux\n");
    printf("Cria um arquivo para emular a memoria\n");

    /* Se init for chamado de novo, fecha o arquivo que já estava aberto. */
    if (f != NULL)
    {
        fclose(f);
        f = NULL;
    }

    f = fopen(NOME_ARQUIVO, "r+");

    /*
     * O arquivo existe, mas com tamanho errado (ex.: o programa caiu
     * enquanto preenchia). Descarta e cria de novo.
     */
    if (f != NULL && tamanho_do_arquivo() != MEMORIA_TAMANHO)
    {
        printf("Arquivo de particao com tamanho errado, recriando\n");
        fclose(f);
        f = NULL;
    }

    if (f == NULL)
    {
        f = fopen(NOME_ARQUIVO, "w+");
        if (f == NULL)
        {
            printf("Impossivel criar arquivo de particao\n");
            return;
        }
        preenche_com_ff();
    }
}

void memoria_fwrite(void *buffer, uint16_t posicao, uint16_t tam)
{
    //printf("Chamou a versao de mem.write do linux\n");

    if (f == NULL)
    {
        printf("Erro: memoria nao inicializada\n");
        return;
    }
    if (!dentro_da_memoria(posicao, tam))
    {
        printf("Erro: escrita fora da memoria (posicao %u, tam %u)\n", posicao, tam);
        return;
    }

    fseek(f, posicao, SEEK_SET);
    if (fwrite(buffer, tam, 1, f) != 1 && tam > 0)
    {
        printf("Erro ao gravar no arquivo de particao\n");
    }

    /* fwrite só coloca o dado no buffer do stdio; fflush manda para o arquivo. */
    fflush(f);
}

void memoria_fread(void *buffer, uint16_t posicao, uint16_t tam)
{
    //printf("Chamou a versao de mem.read do linux\n");

    if (f == NULL)
    {
        printf("Erro: memoria nao inicializada\n");
        return;
    }
    if (!dentro_da_memoria(posicao, tam))
    {
        printf("Erro: leitura fora da memoria (posicao %u, tam %u)\n", posicao, tam);
        return;
    }

    fseek(f, posicao, SEEK_SET);
    if (fread(buffer, tam, 1, f) != 1 && tam > 0)
    {
        printf("Erro ao ler o arquivo de particao\n");
    }
}
