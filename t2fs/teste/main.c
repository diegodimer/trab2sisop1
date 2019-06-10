// ARQUIVO USADO PARA TESTAR O CODIGO !!!!!!!!!!!!!
#include <stdio.h>
#include <stdlib.h>
#include "t2fs.h"



typedef struct
{
    DWORD   proxBloco;     /* Variável para o próximo bloco ocupado por essa entrada */
    DWORD   nBlocosSist;   /* Varíavel com o número de blocos no sistema */
    char    name[231];      /* Nome do arquivo cuja entrada foi lida do disco      */
    DWORD   fileSize;       /* Numero de bytes do arquivo */
    DWORD  *dirFilhos;
} ROOTDIR;

typedef struct
{
    char    name[256];     /* PATH ABSOLUTO DO DIRETÓRIO */
    BYTE    fileType;      /* Tipo do arquivo: regular (0x01) ou diretório (0x02) */
    DWORD   fileSize;      /* Numero de bytes do arquivo */
    DWORD  *dirFilhos;     /* LISTA DE DIRETÓRIOS FILHOS */
    int     numFilhos;
    DWORD   setorDados;    /* PONTEIRO PARA O PRIMEIRO SETOR COM OS DADOS (ARQUIVO REGULAR) */
} DIRENT3;

int main( int argc, char *argv[])
{
    //format2(8);

    mkdir2("/a/BECE");
//    mkdir2("/a/c/e");
//    mkdir2("/a/c/d");
//    mkdir2("/diego");
//    mkdir2("/b/cu");
//    mkdir2("/b/cu2");

    printf("-----lendo bloco------\n");
    unsigned char *buffer =  NULL;
    buffer = (unsigned char *)readBlock(9);
    DIRENT3 *diret = (DIRENT3 *) buffer;
    printf("nome do novo diretorio: %s\n", diret->name);
    printf("file size: %d\n", diret->fileSize);
    printf("file type: %d\n", diret->fileType);
    printf("num filhos: %d\n", diret->numFilhos);
    printf("setor dados: %d\n", diret->setorDados);
    printf("--------------------------\n");

    printf("-----lendo bloco------\n");
    buffer = (unsigned char *)readBlock(17);
    diret = (DIRENT3 *) buffer;
    printf("nome do novo diretorio: %s\n", diret->name);
    printf("file size: %d\n", diret->fileSize);
    printf("file type: %d\n", diret->fileType);
    printf("num filhos: %d\n", diret->numFilhos);
    printf("setor dados: %d\n", diret->setorDados);
    printf("--------------------------\n");
        printf("-----lendo bloco------\n");
    buffer = (unsigned char *)readBlock(1);
    diret = (DIRENT3 *) buffer;
    printf("nome do novo diretorio: %s\n", diret->name);
    printf("file size: %d\n", diret->fileSize);
    printf("file type: %d\n", diret->fileType);
    printf("num filhos: %d\n", diret->numFilhos);
    printf("setor dados: %d\n", diret->setorDados);
    printf("--------------------------\n");
            printf("-----lendo bloco------\n");
    buffer = (unsigned char *)readBlock(25);
    diret = (DIRENT3 *) buffer;
    printf("nome do novo diretorio: %s\n", diret->name);
    printf("file size: %d\n", diret->fileSize);
    printf("file type: %d\n", diret->fileType);
    printf("num filhos: %d\n", diret->numFilhos);
    printf("setor dados: %d\n", diret->setorDados);
    printf("--------------------------\n");



//    ROOTDIR *rootDirectory ;
//    int n;
//    int i;
//    format2(8);
//    while(1)
//    {
//        scanf("%d", &n);
//        unsigned char *buffer = NULL;
//        buffer = (unsigned char*)readBlock(n);
//        if(buffer==NULL){
//            printf("ERro na leitura\n");
//        }
//        char *auxBuff = malloc(sizeof(ROOTDIR));
//        for(i=0; i<sizeof(ROOTDIR);i++){
//            auxBuff[i] = buffer[i];
//        }
//        rootDirectory = (ROOTDIR *) buffer;
//        printf("nome: %s\n", rootDirectory->name);
//        printf("filesize: %d\n", (int)rootDirectory->fileSize);
//        printf("nblocosis: %d\n", (int)rootDirectory->nBlocosSist);
//        printf("proxblock: %d\n", (int)rootDirectory->proxBloco);
//    }

    return 0;
}
