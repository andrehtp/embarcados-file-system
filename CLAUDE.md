# Trabalho SA — Sistema de arquivos tipo inode sobre EEPROM

Trabalho da disciplina Sistemas Digitais Embarcados (UFSC): sistema de arquivos
tipo inode sobre uma EEPROM I2C AT24C512 (64 KiB, páginas de 128 bytes), em
ESP32 com ESP-IDF (`idf.py`). Enunciado completo: `docs/Sistema_Arquivos.pdf`.

## Arquitetura em camadas (a dependência só desce)

```
main (aplicação, camada 5) → sa_api (camada 4: meu_fopen, meu_fread...) → sa (camada 3: inodos, blocos, lista de livres) → memoria (camada 2)
```

- `memoria` é a única camada que conhece o hardware. API: `memoria_init`,
  `memoria_fread`, `memoria_fwrite` (`components/memoria/include/memoria.h`).
- Backend escolhido pelo CMake conforme `IDF_TARGET`:
  - `memoria_linux.c`: emula a EEPROM com o arquivo `PARTICAO.BIN` (64 KiB).
  - `memoria_esp32.c`: I2C real (por enquanto só um stub).
- `testes/` não é componente do IDF; é compilado à parte com `gcc`.

## Regra de trabalho

**O aluno implementa a lógica. O Claude explica, revisa e aponta erros, mas não
escreve a implementação das camadas, a menos que o aluno peça explicitamente.**

## Testes (Linux)

Na raiz do projeto:

```sh
gcc -Wall -g -fsanitize=address -I components/memoria/include \
    components/memoria/memoria_linux.c testes/teste_memoria.c -o teste && ./teste
```

Dica: `./teste | grep -v Chamou` esconde os `printf` do professor.

## Próximos passos

1. Implementar os componentes `sa` (camada 3) e `sa_api` (camada 4). Já existem
   como componentes do IDF, mas com `.c`/`.h` vazios. `sa` depende de `memoria`
   e `sa_api` de `sa` (`PRIV_REQUIRES` no `CMakeLists.txt` de cada um).
2. `memoria_esp32.c` com I2C de verdade.

## Detalhes físicos da AT24C512 (para o backend ESP32)

- Escrita por páginas de 128 bytes: se uma escrita passa do fim da página, o
  endereço "dá a volta" para o início da mesma página e sobrescreve dados.
  Escritas maiores ou desalinhadas precisam ser quebradas nas fronteiras de página.
- Cerca de 5 ms de tempo de gravação por página: esperar (ou fazer polling de
  ACK) antes da próxima operação.
