/*
 * Testes do sistema de arquivos (camadas 3 e 4), versão Linux.
 *
 * Compilar e rodar na raiz do projeto:
 *   gcc -Wall -g -fsanitize=address -I components/memoria/include \
 *       -I components/sa/include -I components/sa_api/include \
 *       components/memoria/memoria_linux.c components/sa/sa.c \
 *       components/sa_api/sa_api.c testes/teste_sa.c -o teste_sa && ./teste_sa
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "memoria.h"
#include "sa.h"
#include "sa_api.h"

/* Conta os blocos livres percorrendo a lista encadeada direto na memória */
static unsigned int conta_livres(void)
{
    uint16_t b;
    unsigned int n = 0;
    memoria_fread(&b, END_CABECALHO, 2);
    while (b != BLOCO_NULO) {
        assert(b < N_BLOCOS);
        n++;
        memoria_fread(&b, END_PONTEIROS + b * TAM_PONTEIRO, 2);
    }
    return n;
}

/* Preenche buf com um padrão que depende da posição */
static void padrao(uint8_t *buf, int tam, int semente)
{
    for (int i = 0; i < tam; i++)
        buf[i] = (uint8_t)(i * 7 + semente);
}

/* 1. Formatação: cabeçalho 0, inodos livres, todos os blocos na lista */
static void teste_formata(void)
{
    formata_SA();

    uint16_t cab;
    memoria_fread(&cab, END_CABECALHO, 2);
    assert(cab == 0);

    inode in;
    for (int k = 0; k < NUM_INODOS; k++) {
        sa_ler_inodo(k, &in);
        assert(in.status == INODO_LIVRE);
        assert(in.ponteiro_indireto == BLOCO_NULO);
    }
    assert(conta_livres() == N_BLOCOS);
    printf("ok: formata\n");
}

/* 2. Arquivo pequeno (só bloco direto) */
static void teste_pequeno(void)
{
    formata_SA();
    char lido[16] = {0};

    ARQUIVO *a = meu_fopen("teste", "w");
    assert(a != NULL);
    assert(meu_fwrite(a, (uint8_t *)"Ola!", 4) == 4);
    assert(meu_ftell(a) == 4);
    meu_fclose(a);

    a = meu_fopen("teste", "r+");
    assert(a != NULL);
    assert(!meu_feof(a));
    assert(meu_fread(a, (uint8_t *)lido, sizeof lido) == 4);   /* só lê o que existe */
    assert(memcmp(lido, "Ola!", 4) == 0);
    assert(meu_feof(a));
    meu_fclose(a);

    assert(conta_livres() == N_BLOCOS);   /* não precisou de bloco */
    assert(meu_fopen("naoexiste", "r+") == NULL);
    printf("ok: arquivo pequeno\n");
}

/* 3. Exemplo do enunciado */
static void teste_exemplo_professor(void)
{
    formata_SA();
    char buffer[70];

    ARQUIVO *B = meu_fopen("Fabio", "w");
    strcpy(buffer, "Oi mundo, teste de mensagemXXX");
    meu_fwrite(B, (uint8_t *)buffer, strlen(buffer));
    strcpy(buffer, " Prof: FABIO");
    meu_fwrite(B, (uint8_t *)buffer, strlen(buffer));
    strcpy(buffer, " de la Rocha UFSC Ararangua");
    meu_fwrite(B, (uint8_t *)buffer, strlen(buffer));
    meu_fclose(B);

    ARQUIVO *C = meu_fopen("Fabio", "r");
    assert(meu_fseek(C, 19, SEEK_SET) == 0);
    assert(meu_fread(C, (uint8_t *)buffer, 8) == 8);
    buffer[8] = 0;
    assert(strcmp(buffer, "mensagem") == 0);

    /* 69 bytes: bloco direto + 2 blocos de dados + 1 indireto */
    assert(meu_fseek(C, 0, SEEK_END) == 0);
    assert(meu_ftell(C) == 69);
    assert(conta_livres() == N_BLOCOS - 3);
    meu_fclose(C);
    printf("ok: exemplo do professor\n");
}

/* 4. Arquivo de tamanho máximo, atravessando fronteiras de bloco */
static void teste_maximo(void)
{
    formata_SA();
    uint8_t escrito[TAM_MAX_ARQUIVO + 10], lido[TAM_MAX_ARQUIVO];
    padrao(escrito, sizeof escrito, 3);

    ARQUIVO *a = meu_fopen("grande", "w");
    /* escreve em pedaços de 50 (desalinhados com os blocos de 32) */
    uint16_t total = 0;
    for (int i = 0; i < (int)sizeof escrito; i += 50) {
        uint16_t n = sizeof escrito - i < 50 ? sizeof escrito - i : 50;
        total += meu_fwrite(a, escrito + i, n);
    }
    assert(total == TAM_MAX_ARQUIVO);   /* o excesso foi recusado */
    assert(meu_fwrite(a, escrito, 1) == 0);

    meu_fseek(a, 0, SEEK_SET);
    assert(meu_fread(a, lido, sizeof lido) == TAM_MAX_ARQUIVO);
    assert(memcmp(escrito, lido, TAM_MAX_ARQUIVO) == 0);

    /* leitura no meio, cruzando blocos */
    meu_fseek(a, 30, SEEK_SET);
    assert(meu_fread(a, lido, 40) == 40);
    assert(memcmp(escrito + 30, lido, 40) == 0);
    assert(meu_fseek(a, 1, SEEK_END) == -1);
    meu_fclose(a);

    assert(conta_livres() == N_BLOCOS - 17);   /* 16 de dados + 1 indireto */
    printf("ok: arquivo maximo\n");
}

/* 5. "w" apaga o conteúdo e remove devolve os blocos */
static void teste_trunca_e_remove(void)
{
    formata_SA();
    uint8_t dados[200];
    padrao(dados, sizeof dados, 9);

    ARQUIVO *a = meu_fopen("arq", "w");
    meu_fwrite(a, dados, sizeof dados);
    meu_fclose(a);
    assert(conta_livres() < N_BLOCOS);

    a = meu_fopen("arq", "w");
    assert(meu_feof(a));
    meu_fclose(a);
    assert(conta_livres() == N_BLOCOS);

    a = meu_fopen("arq", "w");
    meu_fwrite(a, dados, sizeof dados);
    meu_fclose(a);
    assert(meu_remove("arq") == 0);
    assert(meu_fopen("arq", "r+") == NULL);
    assert(meu_remove("arq") == -1);
    assert(conta_livres() == N_BLOCOS);
    printf("ok: trunca e remove\n");
}

/* 6. Só existem 12 inodos */
static void teste_inodos_cheios(void)
{
    formata_SA();
    char nome[9];
    for (int i = 0; i < NUM_INODOS; i++) {
        sprintf(nome, "arq%d", i);
        assert(meu_fopen(nome, "w") != NULL);
    }
    assert(meu_fopen("demais", "w") == NULL);

    meu_remove("arq5");
    assert(meu_fopen("agora", "w") != NULL);   /* reaproveita o inodo livre */
    printf("ok: inodos cheios\n");
}

/* 7. Os dados sobrevivem a um novo inicializa_SA */
static void teste_persistencia(void)
{
    formata_SA();
    uint8_t escrito[100], lido[100];
    padrao(escrito, sizeof escrito, 5);

    ARQUIVO *a = meu_fopen("persist", "w");
    meu_fwrite(a, escrito, sizeof escrito);
    meu_fclose(a);
    unsigned int livres = conta_livres();

    inicializa_SA();   /* "religa" sem formatar */
    a = meu_fopen("persist", "r+");
    assert(a != NULL);
    assert(meu_fread(a, lido, sizeof lido) == sizeof lido);
    assert(memcmp(escrito, lido, sizeof lido) == 0);
    meu_fclose(a);

    /* a lista de livres continua certa: novo arquivo não pisa no antigo */
    a = meu_fopen("outro", "w");
    meu_fwrite(a, escrito, sizeof escrito);
    meu_fclose(a);
    assert(conta_livres() == livres - 4);

    a = meu_fopen("persist", "r+");
    meu_fread(a, lido, sizeof lido);
    assert(memcmp(escrito, lido, sizeof lido) == 0);
    meu_fclose(a);
    printf("ok: persistencia\n");
}

int main(void)
{
    remove("PARTICAO.BIN");   /* começa sempre de uma partição nova */
    inicializa_SA();

    teste_formata();
    teste_pequeno();
    teste_exemplo_professor();
    teste_maximo();
    teste_trunca_e_remove();
    teste_inodos_cheios();
    teste_persistencia();

    lista_todos();
    printf("\nTodos os testes passaram\n");
    return 0;
}
