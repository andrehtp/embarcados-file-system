/*
 * Camada de aplicação: exemplo do enunciado.
 * Esperado: "Lido=mensagem" e a lista com UFSC (30 bytes) e Fabio (69 bytes).
 */

#include <stdio.h>
#include <string.h>
#include "sa_api.h"

void app_main(void)
{
    char buffer[70];
    ARQUIVO *A, *B, *C;

    inicializa_SA();
    formata_SA(); // realizado apenas 1 vez

    A = meu_fopen("UFSC", "w");
    strcpy(buffer, "Oi mundo, teste de mensagemXXX");
    meu_fwrite(A, (uint8_t *)buffer, strlen(buffer));
    meu_fclose(A);

    B = meu_fopen("Fabio", "w");
    strcpy(buffer, "Oi mundo, teste de mensagemXXX");
    meu_fwrite(B, (uint8_t *)buffer, strlen(buffer));

    strcpy(buffer, " Prof: FABIO");
    meu_fwrite(B, (uint8_t *)buffer, strlen(buffer));

    strcpy(buffer, " de la Rocha UFSC Ararangua");
    meu_fwrite(B, (uint8_t *)buffer, strlen(buffer));
    meu_fclose(B);

    C = meu_fopen("Fabio", "r");
    meu_fseek(C, 19, SEEK_SET);
    meu_fread(C, (uint8_t *)buffer, 8);
    buffer[8] = 0;
    printf("Lido=%s\n", buffer);
    meu_fclose(C);

    lista_todos();
}
