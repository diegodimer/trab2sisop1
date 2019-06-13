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
//    char path[100];
//    char path2[100];
//    int b,c;
//    DIR2 handle;
//    format2(8);
//    while(1)
//    {
//        printf("Path: ");
//        fflush(stdin);
//        scanf(" %s", path);
//        printf("0: cria 1: procura 2: abre 3: fecha");
//        scanf("%d",&c);
//        if (c==0)
//        {
//            b=mkdir2(path);
//            printf("ret: %d\n",b);
//        }
//        else if(c==1)
//        {
//            getcwd2(path2, 100);
//            puts(path2);
//            if(lookForDir(path) == NULL)
//                printf("Not found\n");;
//        }
//        else if(c==2)
//        {
//            handle = opendir2(path);
//        }
//        else
//        {
//            closedir2(handle);
//        }
//    }

    format2(8);
    mkdir2("/a/b/c");
    mkdir2("/a/d/e");
    mkdir2("/e/f/g");
    mkdir2("/diego");

    DIRENT2 diretorio;
    int handle = opendir2("/a");
    int handle2 = opendir2("/a");
    int handle3 = opendir2("/a");
    int handle4 = opendir2("/a");
    int handle5 = opendir2("/a");
    readdir2(handle, &diretorio);
    printf("nome: %s type: %x size: %d\n",diretorio.name, diretorio.fileType, diretorio.fileSize);
    return 0;
}
