// ARQUIVO USADO PARA TESTAR O CODIGO !!!!!!!!!!!!!
#include <stdio.h>
#include <stdlib.h>
#include "t2fs.h"



typedef struct
{
    DWORD   nBlocosSist;   /* Varíavel com o número de blocos no sistema */
    char    name[256];     /* PATH ABSOLUTO DO DIRETÓRIO */
    BYTE    fileType;      /* Tipo do arquivo: regular (0x01) ou diretório (0x02) */
    DWORD   fileSize;      /* Numero de bytes do arquivo */
    int		bloco_livre;
    int     numFilhos;
    DWORD   dirFilhos[];     /* LISTA DE DIRETÓRIOS FILHOS */
} ROOTDIR;

typedef struct
{
    char    name[256];     /* PATH ABSOLUTO DO DIRETÓRIO */
    BYTE    fileType;      /* Tipo do arquivo: regular (0x01) ou diretório (0x02) */
    DWORD   fileSize;      /* Numero de bytes do arquivo */
    DWORD   setorDados;    /* PONTEIRO PARA O PRIMEIRO SETOR COM OS DADOS (ARQUIVO REGULAR) */
    int     numFilhos;
    DWORD   dirFilhos[];     /* LISTA DE DIRETÓRIOS FILHOS */
} DIRENT3;

int main( int argc, char *argv[])
{
    char path[100];
    FILE2 handle;
    format2(8);
    getcwd2(path, 100);
    mkdir2("/a");
    printf("%s\n", path);
    chdir2("/a");
    mkdir2("/a");
    printf("%s\n", path);
    printf("%d", create2("caralhoa"));
    unsigned char* demonio = readBlock(17);
    DIRENT3 *arq = (DIRENT3 *)demonio;
    printf("arq name: %s\n", arq->name);
    handle = open2("/a/caralhoa");
    printf("handle: %d\n", handle);
   write2(handle, "batata", strlen("batata"));
    read2(handle, path, strlen("batata"));
    printf("MAIN - LIDO: %s\n", path);
    return 0;
}
