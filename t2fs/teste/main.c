// ARQUIVO USADO PARA TESTAR O CODIGO !!!!!!!!!!!!!
#include <stdio.h>
#include <stdlib.h>
#include "t2fs.h"



typedef struct
{
    DWORD   nBlocosSist;   /* Var�avel com o n�mero de blocos no sistema */
    char    name[256];     /* PATH ABSOLUTO DO DIRET�RIO */
    BYTE    fileType;      /* Tipo do arquivo: regular (0x01) ou diret�rio (0x02) */
    DWORD   fileSize;      /* Numero de bytes do arquivo */
    int		bloco_livre;
    int     numFilhos;
    DWORD   dirFilhos[];     /* LISTA DE DIRET�RIOS FILHOS */
} ROOTDIR;

typedef struct
{
    char    name[256];     /* PATH ABSOLUTO DO DIRET�RIO */
    BYTE    fileType;      /* Tipo do arquivo: regular (0x01) ou diret�rio (0x02) */
    DWORD   fileSize;      /* Numero de bytes do arquivo */
    DWORD   setorDados;    /* PONTEIRO PARA O PRIMEIRO SETOR COM OS DADOS (ARQUIVO REGULAR) */
    int     numFilhos;
    DWORD   dirFilhos[];     /* LISTA DE DIRET�RIOS FILHOS */
} DIRENT3;

int main( int argc, char *argv[])
{
    format2(8);


    return 0;
}
