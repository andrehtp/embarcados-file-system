#ifndef SA_H
#define SA_H

#include <stdint.h>
#include <stdbool.h>

/* ---------- Layout da EEPROM ----------
 * [cabeçalho 2B][12 inodos x 45B][N ponteiros x 2B][N blocos de dados x 32B]
 */
#define TAM_MEMORIA      65536u
#define NUM_INODOS       12
#define TAM_INODO        45
#define TAM_BLOCO        32
#define TAM_PONTEIRO     2
#define PONTEIROS_POR_BLOCO (TAM_BLOCO / TAM_PONTEIRO)            /* 16 */

#define END_CABECALHO    0
#define END_INODOS       (END_CABECALHO + 2)                      /* 2 */
#define END_PONTEIROS    (END_INODOS + NUM_INODOS * TAM_INODO)     /* 542 */
#define N_BLOCOS         ((TAM_MEMORIA - END_PONTEIROS) / (TAM_PONTEIRO + TAM_BLOCO)) /* 1911 */
#define END_DADOS        (END_PONTEIROS + N_BLOCOS * TAM_PONTEIRO) /* 4364 */

#define BLOCO_NULO       0xFFFF  /* fim da lista de livres / "sem bloco" */

#define INODO_LIVRE      0
#define INODO_OCUPADO    1

/* Maior arquivo possível: bloco direto + 16 blocos via indireto = 544 bytes */
#define TAM_MAX_ARQUIVO  (TAM_BLOCO + PONTEIROS_POR_BLOCO * TAM_BLOCO)

/* ---------- Inodo (formato gravado na EEPROM) ---------- */
typedef struct __attribute__((packed)) {
    uint8_t  status;
    char     nome[8];             /* sem '\0' se tiver 8 letras */
    uint16_t tamanho;
    uint8_t  bloco_direto[TAM_BLOCO];
    uint16_t ponteiro_indireto;
} inode;

_Static_assert(sizeof(inode) == TAM_INODO, "inode deve ter 45 bytes");

/* ---------- 1. Inicialização e formatação ---------- */

/* Chama memoria_init e carrega o primeiro bloco livre do cabeçalho para a RAM. */
void sa_init(void);

/* Cabeçalho = 0, todos os inodos livres (ponteiro_indireto = BLOCO_NULO),
 * lista encadeada: ponteiro[i] = i+1, último = BLOCO_NULO. */
void sa_formata(void);

/* ---------- 2. Inodos ---------- */

void sa_ler_inodo(uint8_t k, inode *in);
void sa_gravar_inodo(uint8_t k, const inode *in);

/* Índice do inodo ocupado com esse nome (compara até 8 chars), ou -1. */
int sa_procura_inodo(const char *nome);

/* Índice do primeiro inodo livre, ou -1 se os 12 estiverem ocupados. */
int sa_procura_inodo_livre(void);

/* ---------- 3. Lista de blocos livres ---------- */

/* Tira o primeiro bloco da lista e atualiza o cabeçalho.
 * Retorna o número do bloco, ou BLOCO_NULO se a memória estiver cheia. */
uint16_t sa_aloca_bloco(void);

/* Devolve o bloco para o início da lista e atualiza o cabeçalho. */
void sa_libera_bloco(uint16_t bloco);

/* ---------- 4. Dados de um bloco ---------- */

/* Acessam tam bytes a partir de off dentro de UM bloco (off + tam <= TAM_BLOCO). */
void sa_ler_dados(uint16_t bloco, uint8_t off, uint8_t *buf, uint8_t tam);
void sa_escrever_dados(uint16_t bloco, uint8_t off, const uint8_t *buf, uint8_t tam);

/* ---------- 5. Bloco indireto ---------- */

/* Número na EEPROM do bloco 'indice' (0..15) do arquivo, via bloco indireto.
 * Com alocar = true, cria o indireto e/ou o bloco de dados se faltarem.
 * Retorna BLOCO_NULO se não existir (alocar = false) ou se a memória estiver cheia.
 * Pode alterar in->ponteiro_indireto: quem chamou deve gravar o inodo depois. */
uint16_t sa_bloco_do_arquivo(inode *in, uint8_t indice, bool alocar);

/* ---------- 6. Arquivo ---------- */

/* Libera os blocos de dados e o bloco indireto do arquivo.
 * Deixa ponteiro_indireto = BLOCO_NULO e tamanho = 0 (não grava o inodo). */
void sa_libera_arquivo(inode *in);

#endif
