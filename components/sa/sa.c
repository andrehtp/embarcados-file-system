#include "sa.h"
#include "memoria.h"
#include <stdio.h>
#include <string.h>

static uint16_t ff_block;   /* cópia em RAM do cabeçalho (primeiro bloco livre) */

/* Endereço na EEPROM do ponteiro (lista de livres) do bloco b. */
static uint16_t end_ponteiro(uint16_t b){
    return END_PONTEIROS + b * TAM_PONTEIRO;
}

/* Grava ff_block no cabeçalho. */
static void grava_cabecalho(void){
    memoria_fwrite(&ff_block, END_CABECALHO, 2);
}

/* ---------- 1. Inicialização e formatação ---------- */

void sa_init(void){
    memoria_init();
    memoria_fread(&ff_block, END_CABECALHO, 2);
}

void sa_formata(void){

    ff_block = 0;
    grava_cabecalho();

    inode inodos[NUM_INODOS];
    for(uint16_t i = 0; i < NUM_INODOS; i++){
        memset(&inodos[i], 0, sizeof(inode));
        inodos[i].status = INODO_LIVRE;
        inodos[i].ponteiro_indireto = BLOCO_NULO;
    }
    memoria_fwrite(inodos, END_INODOS, sizeof(inodos));

    static uint16_t ponteiro[N_BLOCOS];

    for(uint16_t i = 0; i < N_BLOCOS; i++){
        ponteiro[i] = i == (N_BLOCOS - 1) ? BLOCO_NULO : i+1;
    }
    memoria_fwrite(ponteiro, END_PONTEIROS, sizeof(ponteiro));
}

/* ---------- 2. Inodos ---------- */

void sa_ler_inodo(uint8_t k, inode *in){
    memoria_fread(in, END_INODOS + k * TAM_INODO, TAM_INODO);
}

void sa_gravar_inodo(uint8_t k, const inode *in){
    memoria_fwrite((void *)in, END_INODOS + k * TAM_INODO, TAM_INODO);
}

int sa_procura_inodo(const char *nome){
    inode in;
    for(uint8_t k = 0; k < NUM_INODOS; k++){
        sa_ler_inodo(k, &in);
        if(in.status == INODO_OCUPADO && strncmp(in.nome, nome, 8) == 0){
            return k;
        }
    }
    return -1;
}

int sa_procura_inodo_livre(void){
    inode in;
    for(uint8_t k = 0; k < NUM_INODOS; k++){
        sa_ler_inodo(k, &in);
        if(in.status == INODO_LIVRE){
            return k;
        }
    }
    return -1;
}

/* ---------- 3. Lista de blocos livres ---------- */

uint16_t sa_aloca_bloco(void){
    if(ff_block == BLOCO_NULO){
        return BLOCO_NULO;   /* memória cheia */
    }

    uint16_t bloco = ff_block;
    uint16_t proximo;
    memoria_fread(&proximo, end_ponteiro(bloco), 2);

    ff_block = proximo;
    grava_cabecalho();
    return bloco;
}

void sa_libera_bloco(uint16_t bloco){
    /* O bloco liberado aponta para o antigo primeiro livre e vira o novo primeiro. */
    memoria_fwrite(&ff_block, end_ponteiro(bloco), 2);
    ff_block = bloco;
    grava_cabecalho();
}

/* ---------- 4. Dados de um bloco ---------- */

void sa_ler_dados(uint16_t bloco, uint8_t off, uint8_t *buf, uint8_t tam){
    memoria_fread(buf, END_DADOS + bloco * TAM_BLOCO + off, tam);
}

void sa_escrever_dados(uint16_t bloco, uint8_t off, const uint8_t *buf, uint8_t tam){
    memoria_fwrite((void *)buf, END_DADOS + bloco * TAM_BLOCO + off, tam);
}

/* ---------- 5. Bloco indireto ---------- */

uint16_t sa_bloco_do_arquivo(inode *in, uint8_t indice, bool alocar){
    if(indice >= PONTEIROS_POR_BLOCO){
        return BLOCO_NULO;
    }

    /* Cria o bloco indireto se ainda não existir */
    if(in->ponteiro_indireto == BLOCO_NULO){
        if(!alocar){
            return BLOCO_NULO;
        }
        uint16_t ind = sa_aloca_bloco();
        if(ind == BLOCO_NULO){
            return BLOCO_NULO;
        }
        /* Todos os 16 ponteiros começam "sem bloco" (0xFFFF) */
        uint8_t vazio[TAM_BLOCO];
        memset(vazio, 0xFF, TAM_BLOCO);
        sa_escrever_dados(ind, 0, vazio, TAM_BLOCO);
        in->ponteiro_indireto = ind;
    }

    uint16_t bloco;
    sa_ler_dados(in->ponteiro_indireto, indice * TAM_PONTEIRO, (uint8_t *)&bloco, TAM_PONTEIRO);

    if(bloco == BLOCO_NULO && alocar){
        bloco = sa_aloca_bloco();
        if(bloco != BLOCO_NULO){
            sa_escrever_dados(in->ponteiro_indireto, indice * TAM_PONTEIRO, (uint8_t *)&bloco, TAM_PONTEIRO);
        }
    }
    return bloco;
}

/* ---------- 6. Arquivo ---------- */

void sa_libera_arquivo(inode *in){
    if(in->ponteiro_indireto != BLOCO_NULO){
        uint16_t ponteiros[PONTEIROS_POR_BLOCO];
        sa_ler_dados(in->ponteiro_indireto, 0, (uint8_t *)ponteiros, TAM_BLOCO);

        for(uint8_t i = 0; i < PONTEIROS_POR_BLOCO; i++){
            if(ponteiros[i] != BLOCO_NULO){
                sa_libera_bloco(ponteiros[i]);
            }
        }
        sa_libera_bloco(in->ponteiro_indireto);
        in->ponteiro_indireto = BLOCO_NULO;
    }
    in->tamanho = 0;
}
