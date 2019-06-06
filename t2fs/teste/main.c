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
} ROOTDIR;


int main( int argc, char *argv[])
{
    ROOTDIR *rootDirectory ;
    int n;
    int i;
    while(1)
    {
        scanf("%d", &n);
        unsigned char *buffer = NULL;
        buffer = (unsigned char*)readBlock(n);
        char *auxBuff = malloc(sizeof(ROOTDIR));
        for(i=0; i<sizeof(ROOTDIR);i++){
            auxBuff[i] = buffer[i];
        }
        rootDirectory = (ROOTDIR *) auxBuff;
        printf("%s\n", rootDirectory->name);
        printf("%d\n", (int)rootDirectory->fileSize);
        printf("%d\n", (int)rootDirectory->nBlocosSist);
        printf("%d\n", (int)rootDirectory->proxBloco);
    }

    return 0;
}
