/*
 * Testes da camada de memória (versão Linux).
 *
 * Compilar e rodar na raiz do projeto:
 *   gcc -Wall -g -fsanitize=address -I components/memoria/include \
 *       components/memoria/memoria_linux.c testes/teste_memoria.c -o teste && ./teste
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "memoria.h"

#define TAM 65536   /* tamanho da AT24C512 */

/* Lê o tamanho do arquivo direto do disco, sem passar pela camada de memória */
static long tamanho_arquivo(void)
{
    FILE *f = fopen("PARTICAO.BIN", "rb");
    if (f == NULL) return -1;
    fseek(f, 0, SEEK_END);
    long t = ftell(f);
    fclose(f);
    return t;
}

/* 1. Partição nova: 64 KiB, toda em 0xFF */
static void teste_particao_nova(void)
{
    assert(tamanho_arquivo() == TAM);

    uint8_t buf[256];
    for (long pos = 0; pos < TAM; pos += sizeof buf) {
        memoria_fread(buf, (uint16_t)pos, sizeof buf);
        for (int i = 0; i < (int)sizeof buf; i++)
            assert(buf[i] == 0xFF);
    }
    printf("ok: particao nova\n");
}

/* 2. O que escrevo é o que leio */
static void teste_ida_e_volta(void)
{
    uint8_t escrito[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t lido[4] = {0};

    memoria_fwrite(escrito, 0x0100, 4);
    memoria_fread(lido, 0x0100, 4);
    assert(memcmp(escrito, lido, 4) == 0);
    printf("ok: ida e volta\n");
}

/* 3. Escrever no meio não altera os bytes vizinhos */
static void teste_vizinhos(void)
{
    uint8_t aa[16], bb[4], lido[16];
    memset(aa, 0xAA, sizeof aa);
    memset(bb, 0xBB, sizeof bb);

    memoria_fwrite(aa, 0x0200, 16);   /* 16 bytes AA */
    memoria_fwrite(bb, 0x0206, 4);    /* 4 bytes BB no meio */
    memoria_fread(lido, 0x0200, 16);

    for (int i = 0; i < 16; i++) {
        if (i >= 6 && i < 10) assert(lido[i] == 0xBB);
        else                  assert(lido[i] == 0xAA);
    }
    printf("ok: vizinhos intactos\n");
}

/* 4. Primeira e última posição da memória funcionam */
static void teste_bordas(void)
{
    uint8_t b, lido;

    b = 0x11;
    memoria_fwrite(&b, 0, 1);
    memoria_fread(&lido, 0, 1);
    assert(lido == 0x11);

    b = 0x22;
    memoria_fwrite(&b, TAM - 1, 1);
    memoria_fread(&lido, TAM - 1, 1);
    assert(lido == 0x22);
    printf("ok: bordas\n");
}

/* 5. Escrita que passaria do fim é recusada e não altera nada */
static void teste_fora_da_faixa(void)
{
    uint8_t lixo[16], lido;
    memset(lixo, 0xEE, sizeof lixo);

    memoria_fwrite(lixo, TAM - 4, 16);   /* passaria 12 bytes do fim */

    memoria_fread(&lido, TAM - 1, 1);
    assert(lido == 0x22);                /* valor gravado no teste anterior */
    assert(tamanho_arquivo() == TAM);    /* arquivo não cresceu */
    printf("ok: fora da faixa\n");
}

/* 6. Dado chega ao disco e sobrevive a um novo memoria_init */
static void teste_persistencia(void)
{
    uint8_t escrito[4] = {1, 2, 3, 4};
    uint8_t lido[4] = {0};

    memoria_fwrite(escrito, 0x0300, 4);

    /* Lê o arquivo por fora: se falhar aqui, faltou fflush */
    FILE *f = fopen("PARTICAO.BIN", "rb");
    assert(f != NULL);
    fseek(f, 0x0300, SEEK_SET);
    assert(fread(lido, 1, 4, f) == 4);
    fclose(f);
    assert(memcmp(escrito, lido, 4) == 0);

    /* "Religa" a memória: o arquivo existente não pode ser recriado */
    memoria_init();
    memset(lido, 0, sizeof lido);
    memoria_fread(lido, 0x0300, 4);
    assert(memcmp(escrito, lido, 4) == 0);
    printf("ok: persistencia\n");
}

int main(void)
{
    remove("PARTICAO.BIN");   /* começa sempre de uma partição nova */
    memoria_init();

    teste_particao_nova();
    teste_ida_e_volta();
    teste_vizinhos();
    teste_bordas();
    teste_fora_da_faixa();
    teste_persistencia();

    printf("\nTodos os testes passaram\n");
    return 0;
}
