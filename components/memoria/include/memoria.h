#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdint.h>

#define MEMORIA_TAMANHO 65536u   /* AT24C512 = 64 KiB */

void memoria_init(void);
void memoria_fread(void *buffer, uint16_t posicao, uint16_t tam);
void memoria_fwrite(void *buffer, uint16_t posicao, uint16_t tam);

#endif