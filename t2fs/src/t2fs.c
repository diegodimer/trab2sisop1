#include "t2fs.h"
#include "apidisk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERRO_LEITURA -1
#define ERRO_ESCRITA -2

#define LAST_BLOCK 0

typedef struct{
    WORD  version;          /* Versão do trabalho */
    WORD  sectorSize;       /* Tamanho do setor em Bytes */
    WORD  tabelaParticoes;  /* Byte inicial da tabela de partições */
    WORD  nParticoes;       /* Quantidade de partições no disco */
    DWORD setorInicioP1;    /* endereço do bloco de início da partição 1 */
    DWORD setorFimP1;       /* endereço do ultimo bloco da partição 1 */
    BYTE  nome[25];         /* Nome da partição 1 */
} MBR;

typedef struct {
    DWORD   proxBloco;     /* Variável para o próximo bloco ocupado por essa entrada */
    DWORD   nBlocosSist;   /* Varíavel com o número de blocos no sistema */
    char    name[231];      /* Nome do arquivo cuja entrada foi lida do disco      */
    DWORD   fileSize;       /* Numero de bytes do arquivo */
} ROOTDIR;


ROOTDIR rootDirectory;
int inicializado = 0;
int debug = 1;
int setoresPorBloco;

int init()
{
    unsigned char buffer[257];
    if(read_sector(1, buffer))
        return ERRO_LEITURA;
    ROOTDIR *dir;
    dir = (ROOTDIR *) buffer;
    if(strlen(dir->name) == 0 || strcmp(dir->name, "root")!=0)
    {
        if(debug == 1)
        {
            printf("Não tem diretório raiz\n");
            printf("Criando diretório raiz...\n");
        }
        // criação do diretório raiz
        strcpy(rootDirectory.name, "root");
        rootDirectory.fileSize   = sizeof(ROOTDIR);
        rootDirectory.proxBloco = LAST_BLOCK;                   // não tem próximo bloco = 0
        rootDirectory.nBlocosSist = setoresPorBloco;

        if(!write_sector(1, (unsigned char*)&rootDirectory))
        {
            if(debug == 1)
                printf("Diretório raiz criado com sucesso!\n");
        }
        else
        {
            printf("ERRO na leitura do diretório raiz\n");
            return ERRO_LEITURA;
        }
    }
    else
    {
        rootDirectory = *dir;
        if(debug == 1)
        {
            printf("Diretório raiz lido com sucesso!\n");
            printf("Root directory name: %s\n", rootDirectory.name);
        }
    }
    inicializado = 1;
    return 0;
}

/*-----------------------------------------------------------------------------
Função:	Informa a identificação dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/
int identify2 (char *name, int size)
{
    if(!inicializado)
    {
        if(init()!=0)
            return ERRO_LEITURA;
    };
    if(size>= strlen("Afonto, Diego, Eduardo"))
    {
        strcpy(name, "Afonso, Diego, Eduardo");
        return 1;
    }

    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Formata logicamente o disco virtual t2fs_disk.dat para o sistema de
		arquivos T2FS definido usando blocos de dados de tamanho
		corresponde a um múltiplo de setores dados por sectors_per_block.
-----------------------------------------------------------------------------*/
int format2 (int sectors_per_block)
{
    unsigned char buffer[257];
    if(read_sector(0, buffer))
        return ERRO_LEITURA;
    MBR* dir; // lê o MBR
    dir = (MBR *) buffer;
    if(debug == 1){
        printf("%s\n", dir->nome);
        printf("Inicio: %04x - dec: %d\n", (dir->setorInicioP1), (int)dir->setorInicioP1);
        printf("Fim: %04x - dec: %d\n", (dir->setorFimP1), (int)dir->setorFimP1);
    }
    if(sectors_per_block < 2 || sectors_per_block % 1024 != 0 || sectors_per_block >= 1024)
    {
        return -1;
    }
    return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um novo arquivo no disco e abrí-lo,
		sendo, nesse último aspecto, equivalente a função open2.
		No entanto, diferentemente da open2, se filename referenciar um
		arquivo já existente, o mesmo terá seu conteúdo removido e
		assumirá um tamanho de zero bytes.
-----------------------------------------------------------------------------*/
FILE2 create2 (char *filename)
{
    // o caminho é absoluto, então filename vem desde o root até onde é, eu entro no root, vejo se
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um arquivo do disco.
-----------------------------------------------------------------------------*/
int delete2 (char *filename)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um arquivo existente no disco.
-----------------------------------------------------------------------------*/
FILE2 open2 (char *filename)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um arquivo.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a leitura de uma certa quantidade
		de bytes (size) de um arquivo.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a escrita de uma certa quantidade
		de bytes (size) de  um arquivo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para truncar um arquivo. Remove do arquivo
		todos os bytes a partir da posição atual do contador de posição
		(current pointer), inclusive, até o seu final.
-----------------------------------------------------------------------------*/
int truncate2 (FILE2 handle)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Altera o contador de posição (current pointer) do arquivo.
-----------------------------------------------------------------------------*/
int seek2 (FILE2 handle, DWORD offset)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um novo diretório.
-----------------------------------------------------------------------------*/
int mkdir2 (char *pathname)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um diretório do disco.
-----------------------------------------------------------------------------*/
int rmdir2 (char *pathname)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para alterar o CP (current path)
-----------------------------------------------------------------------------*/
int chdir2 (char *pathname)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para obter o caminho do diretório corrente.
-----------------------------------------------------------------------------*/
int getcwd2 (char *pathname, int size)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um diretório existente no disco.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *pathname)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para ler as entradas de um diretório.
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um diretório.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (softlink) com
		o nome dado por linkname (relativo ou absoluto) para um
		arquivo ou diretório fornecido por filename.
-----------------------------------------------------------------------------*/
int ln2 (char *linkname, char *filename)
{
    return -1;
}



