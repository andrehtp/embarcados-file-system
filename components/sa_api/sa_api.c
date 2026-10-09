#include "sa_api.h"
#include "sa.h"
#include <string.h>

/* Um "ARQUIVO" por inodo: abrir o mesmo arquivo de novo devolve o mesmo
 * ponteiro (com a posição voltando para 0). Assim não precisa de malloc. */
static ARQUIVO tabela[NUM_INODOS];

void inicializa_SA(void){
    sa_init();
    memset(tabela, 0, sizeof(tabela));
}

void formata_SA(void){
    sa_formata();
    memset(tabela, 0, sizeof(tabela));
}

ARQUIVO *meu_fopen(const char *nome, const char *modo){
    int k = sa_procura_inodo(nome);
    inode in;

    if(strcmp(modo, "w") == 0){
        if(k >= 0){
            /* Já existe: apaga o conteúdo e reaproveita o inodo */
            sa_ler_inodo(k, &in);
            sa_libera_arquivo(&in);
        }
        else{
            k = sa_procura_inodo_livre();
            if(k < 0){
                return NULL;   /* os 12 inodos estão ocupados */
            }
            memset(&in, 0, sizeof(in));
            in.status = INODO_OCUPADO;
            strncpy(in.nome, nome, 8);
            in.tamanho = 0;
            in.ponteiro_indireto = BLOCO_NULO;
        }
        sa_gravar_inodo(k, &in);
    }
    else if(strcmp(modo, "r+") == 0 || strcmp(modo, "r") == 0){
        if(k < 0){
            return NULL;   /* não existe */
        }
    }
    else{
        return NULL;   /* modo não suportado */
    }

    tabela[k].inodo = k;
    tabela[k].posicao = 0;
    tabela[k].em_uso = true;
    return &tabela[k];
}

uint16_t meu_fwrite(ARQUIVO *a, const uint8_t *buf, uint16_t tam){
    if(a == NULL || !a->em_uso){
        return 0;
    }

    inode in;
    sa_ler_inodo(a->inodo, &in);

    uint16_t feito = 0;
    while(feito < tam && a->posicao < TAM_MAX_ARQUIVO){
        uint16_t bloco = a->posicao / TAM_BLOCO;   /* 0 = direto, 1..16 = via indireto */
        uint8_t  off   = a->posicao % TAM_BLOCO;
        uint16_t n     = TAM_BLOCO - off;          /* o que cabe até o fim do bloco */
        if(n > tam - feito){
            n = tam - feito;
        }

        if(bloco == 0){
            memcpy(&in.bloco_direto[off], buf + feito, n);
        }
        else{
            uint16_t b = sa_bloco_do_arquivo(&in, bloco - 1, true);
            if(b == BLOCO_NULO){
                break;   /* memória cheia */
            }
            sa_escrever_dados(b, off, buf + feito, n);
        }

        feito += n;
        a->posicao += n;
        if(a->posicao > in.tamanho){
            in.tamanho = a->posicao;
        }
    }

    sa_gravar_inodo(a->inodo, &in);
    return feito;
}

uint16_t meu_fread(ARQUIVO *a, uint8_t *buf, uint16_t tam){
    if(a == NULL || !a->em_uso){
        return 0;
    }

    inode in;
    sa_ler_inodo(a->inodo, &in);

    /* Não lê além do fim do arquivo */
    if(a->posicao >= in.tamanho){
        return 0;
    }
    if(tam > in.tamanho - a->posicao){
        tam = in.tamanho - a->posicao;
    }

    uint16_t feito = 0;
    while(feito < tam){
        uint16_t bloco = a->posicao / TAM_BLOCO;
        uint8_t  off   = a->posicao % TAM_BLOCO;
        uint16_t n     = TAM_BLOCO - off;
        if(n > tam - feito){
            n = tam - feito;
        }

        if(bloco == 0){
            memcpy(buf + feito, &in.bloco_direto[off], n);
        }
        else{
            uint16_t b = sa_bloco_do_arquivo(&in, bloco - 1, false);
            if(b == BLOCO_NULO){
                break;
            }
            sa_ler_dados(b, off, buf + feito, n);
        }

        feito += n;
        a->posicao += n;
    }
    return feito;
}

int meu_fseek(ARQUIVO *a, int32_t desloc, int origem){
    if(a == NULL || !a->em_uso){
        return -1;
    }

    inode in;
    sa_ler_inodo(a->inodo, &in);

    int32_t nova;
    if(origem == SEEK_SET){
        nova = desloc;
    }
    else if(origem == SEEK_CUR){
        nova = a->posicao + desloc;
    }
    else if(origem == SEEK_END){
        nova = in.tamanho + desloc;
    }
    else{
        return -1;
    }

    if(nova < 0 || nova > in.tamanho){
        return -1;
    }
    a->posicao = nova;
    return 0;
}

uint16_t meu_ftell(ARQUIVO *a){
    if(a == NULL || !a->em_uso){
        return 0;
    }
    return a->posicao;
}

bool meu_feof(ARQUIVO *a){
    if(a == NULL || !a->em_uso){
        return true;
    }
    inode in;
    sa_ler_inodo(a->inodo, &in);
    return a->posicao >= in.tamanho;
}

void meu_fclose(ARQUIVO *a){
    if(a != NULL){
        a->em_uso = false;
    }
}

int meu_remove(const char *nome){
    int k = sa_procura_inodo(nome);
    if(k < 0){
        return -1;
    }

    inode in;
    sa_ler_inodo(k, &in);
    sa_libera_arquivo(&in);
    in.status = INODO_LIVRE;
    sa_gravar_inodo(k, &in);

    tabela[k].em_uso = false;
    return 0;
}

void lista_todos(void){
    inode in;
    printf("Arquivos:\n");
    for(uint8_t k = 0; k < NUM_INODOS; k++){
        sa_ler_inodo(k, &in);
        if(in.status == INODO_OCUPADO){
            /* %.8s: o nome pode ter 8 letras sem '\0' */
            printf("  %-8.8s %5u bytes\n", in.nome, in.tamanho);
        }
    }
}
