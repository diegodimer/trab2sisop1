
#include "t2fs.h"
#include "apidisk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERRO_LEITURA -1
#define ERRO_ESCRITA -2

DIRENT2 rootDirectory;
int inicializado = 0;
int debug = 1;
/*-----------------------------------------------------------------------------
Função:	Informa a identificação dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/


int init()
{
    unsigned char buffer[257];
    if(read_sector(1, buffer))
        return ERRO_LEITURA;
    DIRENT2 *dir;
    dir = (DIRENT2 *) buffer;
    if(strlen(dir->name) == 0 || strcmp(dir->name, "Root Directory")!=0)
    {
        if(debug == 1)
        {
            printf("Não tem diretório raiz\n");
            printf("Criando diretório raiz...\n");
        }
        // criação do diretório raiz
        strcpy(rootDirectory.name, "Root Directory");
        rootDirectory.fileSize   = sizeof(DIRENT2);///256*sectors_per_block    // tem o tamanho de um bloco
        rootDirectory.ultimoBloco= 1;                  // está vazio então esse é o último
        rootDirectory.proxBloco = 1;                   // se está sendo criado, o diretório raiz está vazio
        rootDirectory.fileType   = 0x02;               // tipo diretório
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
            printf("File size: %d\nFile type: %d\nLast Sector: %d\nNext Sector: %d\n", (int)rootDirectory.fileSize, (int)rootDirectory.fileType, (int)rootDirectory.ultimoBloco, (int)rootDirectory.proxBloco);
        }
    }
    inicializado = 1;
    return 0;
}


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



