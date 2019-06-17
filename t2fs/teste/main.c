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
    char path2[100];
    int op;
    DIRENT2 dir;


    while(1)
    {
        printf("0- novo dir, 1- novo link, 2- abrir arq, 3- read dir: ");
        scanf("%d", &op);
        printf("path: ");
        scanf(" %s", path);
        if(op==0)
            mkdir2(path);
        else if (op == 1)
        {
            printf("Link de que dir? ");
            scanf(" %s", path2);
            ln2(path, path2);
        }
        else if(op == 2)
        {
            printf("handle: %d\n", opendir2(path));
        } else if(op==3){
            printf("handle: ");
            scanf(" %d", &op);
            readdir2(op, &dir);
            printf("dir name: %s\n", dir.name);
        }

    }


    return 0;
}
