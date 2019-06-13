#include "t2fs.h"
#include "apidisk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERRO_LEITURA -1
#define ERRO_ESCRITA -2

#define INVALID_PATH -3

#define LAST_BLOCK 0

#define ARQ_REGULAR 0x01
#define ARQ_DIRETORIO 0x02

#define MAX_DIR_OPEN 3

typedef struct
{
    WORD  version;          /* Vers�o do trabalho */
    WORD  sectorSize;       /* Tamanho do setor em Bytes */
    WORD  tabelaParticoes;  /* Byte inicial da tabela de parti��es */
    WORD  nParticoes;       /* Quantidade de parti��es no disco */
    DWORD setorInicioP1;    /* endere�o do bloco de in�cio da parti��o 1 */
    DWORD setorFimP1;       /* endere�o do ultimo bloco da parti��o 1 */
    BYTE  nome[25];         /* Nome da parti��o 1 */
} MBR;

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

ROOTDIR *rootDirectory;
int inicializado = 0;
int debug = 1;
DWORD setoresPorBloco=2;
int bloco_livre =0;



// Fun��es auxiliares
int writeBlock(unsigned char *, DWORD); // fun��o que escreve em um bloco
unsigned char* readBlock(DWORD); // fun��o que l� um bloco e retorna um ponteiro para esse bloco
int aloca_bloco(); // fun��o que aloca um bloco
void libera_bloco(int ); // fun��o que desaloca um bloco (preenche com 0s)
int init(); // fun��o que inicia o sistema (preenche variaveis necess�rias para execu��o
DIRENT3 *lookForDir(char*); // fun��o que procura um diret�rio (n�o deve ser chamada por usu�rios)

// vari�veis relativas ao current path
char *currentPath=NULL;
DIRENT3 *currentDir;
int raiz; // se est� no diret�rio raiz


DIRENT3 *openDirectories[MAX_DIR_OPEN];
int dirIndex;


// inicializa o sistema (preenche o diret�rio raiz, se n�o tiver cria. Supoem que o disco est� formatado (variavel setoresporbloco preenchida)
int init()
{
    unsigned char *buffer;
    inicializado = 1;
    buffer = readBlock(1);

    if(setoresPorBloco == 2)
    {
        char conteudo_ultimo[] = {buffer[0],buffer[1],buffer[2],buffer[3]};
        int *stblk = (int *)conteudo_ultimo;
        setoresPorBloco = *stblk;
        buffer = readBlock(1);
    }
    if(buffer == NULL)
    {
        return ERRO_LEITURA;
    }
    int quantos_filhos =(256 * setoresPorBloco) - 265 - 3;

    rootDirectory = malloc(sizeof(*rootDirectory)+quantos_filhos);
    rootDirectory = (ROOTDIR *) buffer;

    if(strcmp(rootDirectory->name, "root")!=0)
    {
        if(debug == 1)
        {
            printf("Nao tem diretorio raiz\n");
            printf("Criando diret�rio raiz...\n");
        }

        free(rootDirectory);
        // cria��o do diret�rio raiz
        rootDirectory = malloc(sizeof(*rootDirectory)+quantos_filhos);
        strcpy(rootDirectory->name, "root");
        rootDirectory->fileSize    = sizeof(*rootDirectory)+quantos_filhos;
        rootDirectory->fileType    = ARQ_DIRETORIO;
        rootDirectory->nBlocosSist = setoresPorBloco;
        rootDirectory->numFilhos   = 0;
        rootDirectory->bloco_livre = bloco_livre;

        if(!writeBlock((unsigned char*)rootDirectory, 1))
        {
            if(debug == 1)
                printf("Diretorio raiz criado com sucesso!\n");
        }
        else
        {
            printf("ERRO na escrita do diretorio raiz\n");
            return ERRO_LEITURA;
        }
    }
    else
    {

        if(debug == 1)
        {
            printf("Diretorio raiz lido com sucesso!\n");
            printf("Root directory name: %s\n", rootDirectory->name);
        }

        setoresPorBloco = rootDirectory->nBlocosSist;
        bloco_livre = rootDirectory->bloco_livre;

    }

    dirIndex=0;
    int i=0;
    for(i=0; i<MAX_DIR_OPEN; i++)
        openDirectories[i]=NULL;

    return 0;
}

/** ESSA FUN��O N�O CUIDA SE PODE OU N�O ESCREVER NO BLOCO, NEM A CORRETUDE (FIRSTSECTOR SENDO MULTIPLO DE 2)!
 ELA APENAS ESCREVE **/
// escreve data no bloco que come�a no setor firstSector
int writeBlock(unsigned char *data, DWORD firstSector)
{
    if (firstSector == -1)
        return -1;
    if(!inicializado)
    {
        init();
    }
    int index = 0;
    int i,j;
    unsigned char aux[SECTOR_SIZE] = {0};
    for(i=0; i<setoresPorBloco; i++) // pra cada setor
    {
        for(j=0; j<SECTOR_SIZE; j++)  // monta o aux com o tamanho do setor
        {
            aux[j] = data[index+j];
        }
        if(debug == 1)
            printf("escrevendo no setor %d\n", firstSector+i);
        index += SECTOR_SIZE; // pra come�ar de data um setor a frente sempre
        write_sector( firstSector+i, aux );

    }
    return 0;
}

// le um bloco da mem�ria
unsigned char* readBlock(DWORD firstSector)
{

    if(firstSector<1)
    {
        return NULL; // se quer o bloco 0 ou negativo, n�o existe (come�a em 1 o n�mero de blocos)
    }

    unsigned char *buffer; // vari�vel preenchida com o conte�do do bloco

    int tamanhoRetorno = (SECTOR_SIZE*setoresPorBloco); // bloco � setorsize*nsetoresporbloco
    buffer = calloc(tamanhoRetorno, sizeof(unsigned char));
    int index = 0;
    int i=0;
    int j;
    unsigned char* auxBuffer = malloc(SECTOR_SIZE);
    while(i!=setoresPorBloco)
    {
        if(debug == 1)
        {
            printf("Lendo setor %d\n", firstSector+i);
        }
        read_sector(firstSector+i, auxBuffer);
        for(j=0; j<SECTOR_SIZE; j++)
        {
            buffer[index] = auxBuffer[j];
            index++;
        }
        i++;
    }
    free(auxBuffer);
    return buffer;
}


/*-----------------------------------------------------------------------------
Fun��o:	Informa a identifica��o dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/
int identify2 (char *name, int size)
{
    if(!inicializado)
    {
        if(init()!=0)
            return ERRO_LEITURA;
    };
    if(size>= strlen("Afonso, Diego, Eduardo"))
    {
        strcpy(name, "Afonso, Diego, Eduardo");
        return 1;
    }

    return -1;
}

//aloca um bloco na memoria, retorna O SETOR INICIAL DO BLOCO
int aloca_bloco()
{

    if(rootDirectory->bloco_livre==0)
    {
        printf("\n nao ha mais blocos disponiveis! \n");
        return 0;
    }
    unsigned char buffer[SECTOR_SIZE];

    read_sector(rootDirectory->bloco_livre,buffer);

    int novo_bloco_livre = buffer[1]<<8 | buffer[0];
    int retorno = rootDirectory->bloco_livre;
    rootDirectory->bloco_livre = novo_bloco_livre;

    writeBlock((unsigned char*)rootDirectory, 1);
    return retorno;


}
//libera um bloco ocupado, esse bloco se junta � linked list dos blocos livres
void libera_bloco(int id_bloco)
{
    unsigned char vetor0[SECTOR_SIZE]= {0};
    //salva o endere�o do antigo bloco livre no bloco passado como parametro
    vetor0[1] = (rootDirectory->bloco_livre>>8);
    vetor0[0] = rootDirectory->bloco_livre;
    if(write_sector(id_bloco,vetor0)!=0)
        printf("erro ao liberar bloco ! \n");
    else
        rootDirectory->bloco_livre = id_bloco;
}


/*-----------------------------------------------------------------------------
Fun��o:	Formata logicamente o disco virtual t2fs_disk.dat para o sistema de
		arquivos T2FS definido usando blocos de dados de tamanho
		corresponde a um m�ltiplo de setores dados por sectors_per_block.
-----------------------------------------------------------------------------*/
int format2 (int sectors_per_block)
{
    if(sectors_per_block < 2 || sectors_per_block % 2 != 0 || sectors_per_block >= 1024)
    {
        return -1;
    }
    //lendo o endere�o da parti��o a ser formatada e transformando de bytes para int
    unsigned char conteudo_mbr[SECTOR_SIZE];
    if(read_sector(0,conteudo_mbr)!=0)
        printf("\n erro ao ler mbr \n");
    char conteudo_bytes[]= {conteudo_mbr[8],conteudo_mbr[9],conteudo_mbr[10],conteudo_mbr[11]};
    int *end_part = (int *)conteudo_bytes;

    //lendo o ultimo bloco l�gico (setor) da parti��o a ser formatada e transformando de bytes para int
    char conteudo_ultimo[] = {conteudo_mbr[12],conteudo_mbr[13],conteudo_mbr[14],conteudo_mbr[15]};
    int *end_ultimosetor = (int *)conteudo_ultimo;

    unsigned char vetorzeros[SECTOR_SIZE]= {0};

    //escreve zeros em todos os setores da parti��o
    int i;
    for(i=*end_part; i<=*end_ultimosetor; i++)
    {
        if(write_sector(i,vetorzeros)!=0)
        {
            printf("\n erro na formata��o dos setores \n");
            return -2;
        }
    }

    //escreve o endere�o do pr�ximo bloco livre nos 2 primeiros bytes de cada bloco

    for(i= *end_part; i<*end_ultimosetor-sectors_per_block; i=i+sectors_per_block)
    {

        int prox_bloco= i + sectors_per_block;
        vetorzeros[1]=(prox_bloco>>8);
        vetorzeros[0]=prox_bloco;
        //printf(" it: %d numero sendo salvo %02x %02x \n",i,vetorzeros[0]&0xFF,vetorzeros[1]&0xFF);
        //char numint[2]={vetorzeros[0],vetorzeros[1]};
        //int gg = *(int *)numeroemint;
        //printf(" numero em int: %d \n",gg);
        if(write_sector(i,vetorzeros)!=0)
        {
            printf("\n erro na formata��o dos blocos livres\n");
            return -3;
        }

    }
    //define a variavel global do inicio da linked list de blocos livres como o bloco 2
    bloco_livre = *end_part + sectors_per_block;
    setoresPorBloco = sectors_per_block;
    inicializado=0;
    init();


    //!!!!!!!!!!!!!!!!!!!!!ATEN��O!!!!!!!!!!!!!!!!!!!!!
    //UNICO COMANDO QUE TRANSFORMA OS BYTES DO ARQUIVO EM INT CORRETAMENTE:
    // int numero_do_setor = buffer[1]<<8 | buffer[0];

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!ATEN��O!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //!!!!!!!!!!!!FOI DEFINIDO ARBITRARIAMENTE QUE UM PONTEIRO PARA BLOCO LIVRE COM VALOR 0 SIGNIFICA QUE N�O H� MAIS BLOCOS LIVRES DISPONIVEIS!!!!!!!!!!

    return 0;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para criar um novo arquivo no disco e abr�-lo,
		sendo, nesse �ltimo aspecto, equivalente a fun��o open2.
		No entanto, diferentemente da open2, se filename referenciar um
		arquivo j� existente, o mesmo ter� seu conte�do removido e
		assumir� um tamanho de zero bytes.
-----------------------------------------------------------------------------*/
FILE2 create2 (char *filename)
{
    // o caminho � absoluto, ent�o filename vem desde o root at� onde �, eu entro no root, vejo se
    return -1;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para remover (apagar) um arquivo do disco.
-----------------------------------------------------------------------------*/
int delete2 (char *filename)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o que abre um arquivo existente no disco.
-----------------------------------------------------------------------------*/
FILE2 open2 (char *filename)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para fechar um arquivo.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para realizar a leitura de uma certa quantidade
		de bytes (size) de um arquivo.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para realizar a escrita de uma certa quantidade
		de bytes (size) de  um arquivo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para truncar um arquivo. Remove do arquivo
		todos os bytes a partir da posi��o atual do contador de posi��o
		(current pointer), inclusive, at� o seu final.
-----------------------------------------------------------------------------*/
int truncate2 (FILE2 handle)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Fun��o:	Altera o contador de posi��o (current pointer) do arquivo.
-----------------------------------------------------------------------------*/
int seek2 (FILE2 handle, DWORD offset)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para criar um novo diret�rio.
-----------------------------------------------------------------------------*/
int mkdir2 (char *pathname)
{

    if(!inicializado)
    {
        if(init()!=0)
            return ERRO_LEITURA;
    };


    if(pathname[0] != '/' || strlen(pathname) < 2) // o caminho � sempre absoluto, ent�o se n�o entrou / no in�cio n�o t� tentando acessar diret�rio raiz
    {
        printf("pathname incorreto, uso: /path_to_dir\n");
        return INVALID_PATH;
    };

    char *pathname2 = strdup(pathname); // n�o da pra pegar direto de pathname
    char *dir = strtok(pathname2, "/"); // o que procurar no diret�rio raiz

    // bloco onde salvar o novo diret�rio
    DWORD novoSubDiretorioBloco;
    DWORD novoDiretorioBloco;
    // novo diret�rio
    DIRENT3 *novoDiretorio = NULL;
    DIRENT3 *novoSubDiretorio = NULL;
    DIRENT3 *dirAuxiliar = NULL;

    int quantos_filhos =(256 * setoresPorBloco) - 265 - 3; // quantos filhos cada diret�rio pode ter
    unsigned char *buffer = NULL;

    int m=0;
    int n=0;
    int encontrado = 0;

    while(rootDirectory->numFilhos > n && encontrado == 0)  // procuro nos filhos do diret�rio raiz o filho dir
    {
        novoDiretorioBloco = rootDirectory->dirFilhos[n]; // o bloco onde o novoDiretorio est� � no diret�rio raiz
        buffer = readBlock(novoDiretorioBloco);
        novoDiretorio = (DIRENT3 *)buffer;


        // comparar o diret�rio que estou (novodiretorio) com o que quero criar
        if(!strcmp(novoDiretorio->name, dir) && novoDiretorio->fileType == ARQ_DIRETORIO)
        {
            // se eles forem iguais encontrei
            encontrado = 1;
            if(debug == 1)
            {
                printf("Encontrei %s no diretorio raiz!\n", dir);
            }
        }
        else
        {
            n++;

        }

    }

    // se o diret�rio raiz tem 0 filhos ou eu n�o encontrei, preciso criar dentro do diret�rio raiz o dir
    if(encontrado == 0)
    {
        novoDiretorioBloco = aloca_bloco();
        // se n�o tem filhos no raiz, criar um
        if(novoDiretorioBloco == 0)
        {
            return -1;

        }

        if(debug == 1)
        {
            printf("Nao encontrei %s no diretorio raiz\nCriando ele como %d filho do raiz\n", dir, rootDirectory->numFilhos);
            printf("Criando filho do diretorio raiz no bloco %d\n", novoDiretorioBloco);
        }
        // se o diret�rio raiz ja tem o tamanho de um bloco em bytes n�o pode alocar mais um diret�rio

        // preencho os dados do novo diret�rio
        novoDiretorio = malloc(sizeof(*novoDiretorio)+quantos_filhos);
        novoDiretorio->dirFilhos[0] = -1;
        novoDiretorio->fileSize     = sizeof(*novoDiretorio)+quantos_filhos;
        novoDiretorio->fileType     = ARQ_DIRETORIO;
        novoDiretorio->numFilhos    = 0;
        novoDiretorio->setorDados   = LAST_BLOCK;
        strcpy(novoDiretorio->name, dir);

        // atualizo o diret�rio raiz
        if(rootDirectory->numFilhos >= (quantos_filhos/2))  // se n�o tem filhos ja t� alocado a mem�ria pra um filho
        {
            printf("max de filhos atingido!\n");
            rootDirectory->dirFilhos[0] = novoDiretorioBloco;
        }
        else
        {
            rootDirectory->dirFilhos[rootDirectory->numFilhos] = novoDiretorioBloco;
        }
        rootDirectory->numFilhos = rootDirectory->numFilhos + 1; // atualizo o n�mero de filhos


        if(!writeBlock((unsigned char *)rootDirectory, 1)) // salvo o diret�rio raiz na mem�ria
        {
            if(!writeBlock((unsigned char*)novoDiretorio, novoDiretorioBloco)) // salvo o novo diretorio na mem�ria
            {
                printf("Diretorio %s criado com sucesso!\n", dir);
            }
        }
    }

    dir = strtok(NULL, "/");
    encontrado = 0;
    m=0;
    while(dir != NULL) // enquanto ainda tiver subdiret�rios
    {
        encontrado = 0;
        m=0;
        // procurar em novoDiretorio a entrada com nome dir
        // se encontrar -> ela � novoDiretorio
        // se n�o encontrar -> criar ela e ela � novoDiretorio
        while(m < novoDiretorio->numFilhos && encontrado == 0)  // itera por todos os filhos do novo dirret�rio ou at� encontrar
        {
            printf("iterando pelos filhos de %s\n", novoDiretorio->name);
            //procurar em novoDiretorio os filhos at� achar algum com nome de dir
            // le o bloco
            buffer = readBlock(novoDiretorio->dirFilhos[m]);
            dirAuxiliar = (DIRENT3 *) buffer;
            // compara o dirAuxiliar com o que estou procurando
            if(!strcmp(dirAuxiliar->name, dir) && dirAuxiliar->fileType == ARQ_DIRETORIO)
            {
                if(debug == 1)
                {
                    printf("Encontrado %s como subdiretorio de %s\n", dir, novoDiretorio->name);
                }
                encontrado = 1;
                // se for igual encontrei
                novoDiretorioBloco = novoDiretorio->dirFilhos[m];
                novoDiretorio = dirAuxiliar;
                // ele � o novoDiretorio, para procurar o prox�mo token nele
            }
            else
            {
                m++;
            }
        }
        m=0; // para pr�xima itera��o

        // se acabou o loop e n�o encontrou
        if (encontrado == 0)
        {
            novoSubDiretorioBloco = (DWORD)aloca_bloco(); // aloca bloco pra guardar o novo diret�rio

            if(novoSubDiretorioBloco == 0)  // se n�o consegue alocar bloco n�o pode criar
            {
                return -1;
            }
            if( novoDiretorio->numFilhos  >= (quantos_filhos/2 ))
            {
                printf("max de filhos atingido!\n");
                return -2; // n�o tem mais espa�o para diret�rios filhos retorna erro
            }


            novoSubDiretorio = malloc( sizeof(*novoSubDiretorio) + quantos_filhos);
            novoSubDiretorio->fileSize     = sizeof(*novoSubDiretorio) + quantos_filhos;
            novoSubDiretorio->fileType     = ARQ_DIRETORIO;
            novoSubDiretorio->numFilhos    = 0;
            novoSubDiretorio->setorDados   = LAST_BLOCK;
            novoSubDiretorio->dirFilhos[0] = -1;
            strcpy(novoSubDiretorio->name,dir);

            // criar em novoDiretorio a entrada dir
            // e faz de dir o novoDiretorio

            novoDiretorio->dirFilhos[novoDiretorio->numFilhos] = novoSubDiretorioBloco;
            novoDiretorio->numFilhos = novoDiretorio->numFilhos + 1;

            if(!writeBlock((unsigned char *)novoDiretorio, novoDiretorioBloco)) // salvo o diret�rio raiz na mem�ria
            {
                if(!writeBlock((unsigned char*)novoSubDiretorio, novoSubDiretorioBloco)) // salvo o novo diretorio na mem�ria
                {
                    printf("Diretorio %s criado com sucesso!\n%s eh subdiretorio de %s\n", novoSubDiretorio->name, novoSubDiretorio->name, novoDiretorio->name);
                }
            }
            *novoDiretorio = *novoSubDiretorio;
            novoDiretorioBloco = novoSubDiretorioBloco;
        }

        dir = strtok(NULL, "/");
        encontrado = 0;
    }

    free(buffer);
    return 0;

}


/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para remover (apagar) um diret�rio do disco.
-----------------------------------------------------------------------------*/
int rmdir2 (char *pathname)
{
    return -1;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para alterar o CP (current path)
-----------------------------------------------------------------------------*/
int chdir2 (char *pathname)
{

    if(!inicializado)
    {
        if(init()!=0)
            return ERRO_LEITURA;
    };
    if(!pathname || strcmp(pathname, "/") == 0) // s� cd no linux leva pro diret�rio raiz
    {
        if(debug==1)
        {
            printf("Ir para o dir. raiz\n");
        }
        free(currentPath);
        currentPath = malloc(sizeof(char)*2);
        strcpy(currentPath, "/");
        raiz = 1;
        return 0;
    }

    int sucesso = 0;
    DIRENT3 *pathDir = lookForDir(pathname);
    if(debug)
        printf("CURRENT PATH NO DIRETORIO %s\n", pathDir->name);

    if(pathDir==NULL)
    {
        sucesso = 0;
    }
    else
    {
        sucesso = 1;
    }
    if(sucesso) // se eu achei o diret�rio pathname � sucesso, newpath � pathname
    {
        char *newPath = calloc(strlen(pathname), sizeof(char));
        if(newPath==NULL)
            return -1;
        strcpy(newPath, pathname);
        free(currentPath);
        currentPath = newPath;
    }
    return 0;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para obter o caminho do diret�rio corrente.
-----------------------------------------------------------------------------*/
int getcwd2 (char *pathname, int size)
{
    if(!inicializado)
    {
        init();
    }
    if(currentPath == NULL)
    {
        currentPath = malloc(sizeof(char)*2);
        strcpy(currentPath, "/");
    }
    if(strlen(currentPath)>size)
        return -1;
    else
        strcpy(pathname, currentPath);
    return 0;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o que abre um diret�rio existente no disco.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *pathname)
{
    if(!inicializado)
    {
        init();
    }
    if(dirIndex >= MAX_DIR_OPEN)
    {
        printf("muitos diretorios abertos\n");
        return -1;
    }
    DIRENT3 *dirToOpen = lookForDir(pathname);

    if(dirToOpen == NULL)
    {
        return -1;
    }
    else
    {
        if(dirToOpen->fileType == ARQ_DIRETORIO)
        {
            openDirectories[dirIndex] = dirToOpen;
            printf("Opened file %s\n", openDirectories[dirIndex]->name);
            dirIndex++;
        }

        return 0;
    }

}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para ler as entradas de um diret�rio.
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry)
{
        if(!inicializado)
    {
        init();
    }

    if(openDirectories[handle] != NULL)
    {
        dentry->fileSize = sizeof(DIRENT2) + openDirectories[handle]->numFilhos;
        dentry->fileType = openDirectories[handle]->fileType;
        strcpy(dentry->name, openDirectories[handle]->name);
        return 0;
    }
    else
    {
        return -1;
    }
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para fechar um diret�rio.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle)
{
        if(!inicializado)
    {
        init();
    }
    if(openDirectories[handle] != NULL)
    {
        printf("Closed file %s\n", openDirectories[handle]->name);
        free(openDirectories[handle]);
        openDirectories[handle] = NULL;
        dirIndex--;
    }
    return 0;
}

/*-----------------------------------------------------------------------------
Fun��o:	Fun��o usada para criar um caminho alternativo (softlink) com
		o nome dado por linkname (relativo ou absoluto) para um
		arquivo ou diret�rio fornecido por filename.
-----------------------------------------------------------------------------*/
int ln2 (char *linkname, char *filename)
{
    return -1;
}

DIRENT3 *lookForDir(char* path)
{
    if(path[0] != '/') // o caminho � sempre absoluto, ent�o se n�o entrou / no in�cio n�o t� tentando acessar diret�rio raiz
    {
        printf("pathname incorreto, uso: /path_to_dir\n");
        return NULL;
    };

    char *pathname2 = strdup(path); // n�o da pra pegar direto de pathname
    char *dir = strtok(pathname2, "/"); // o que procurar no diret�rio raiz


    DWORD novoDiretorioBloco; // bloco onde est� o novo diret�rio lido
    DIRENT3 *novoDiretorio = NULL; // novo diret�rio lido
    DIRENT3 *dirAuxiliar = NULL; // diret�rio auxiliar na procura de subdiretorios
    unsigned char *buffer = NULL; // buffer pra leitura de blocos

    // contadores e flag
    int m=0;
    int n=0;
    int encontrado = 0;

    while(rootDirectory->numFilhos > n && encontrado == 0)  // procuro nos filhos do diret�rio raiz o filho dir
    {
        novoDiretorioBloco = *(rootDirectory->dirFilhos+n); // o bloco onde o novoDiretorio est� � no diret�rio raiz
        buffer = readBlock(novoDiretorioBloco);
        novoDiretorio = (DIRENT3 *)buffer;

        // comparar o diret�rio que estou (novodiretorio) com o que quero criar
        if(!strcmp(novoDiretorio->name, dir))
        {
            // se eles forem iguais encontrei
            encontrado = 1;
            if(debug == 1)
            {
                printf("Encontrei %s no diretorio raiz!\n", novoDiretorio->name);
            }
        }
        else
        {
            n++;

        }

    }

    // se n�o encontrei retorna null e desaloca mem�ria do buffer
    if(encontrado == 0)
    {
        if(debug == 1)
        {
            printf("Diret�rio %s nao encontrado no diretorio raiz!\n", dir);
        }
        free(buffer);
        return NULL;
    }
    else // se encontrei procura o pr�ximo subdiret�rio
    {
        dir = strtok(NULL, "/");
        encontrado = 0;
        m=0;
        while(dir != NULL) // enquanto ainda tiver subdiret�rios
        {
            // procurar em novoDiretorio a entrada com nome dir
            // se encontrar -> ela � novoDiretorio
            while(m < novoDiretorio->numFilhos && encontrado == 0)  // itera por todos os filhos do novo dirret�rio ou at� encontrar
            {
                //procurar em novoDiretorio os filhos at� achar algum com nome de dir
                // le o bloco
                buffer = readBlock(novoDiretorio->dirFilhos[m]);
                dirAuxiliar = (DIRENT3 *) buffer;
                // compara o dirAuxiliar com o que estou procurando
                if(!strcmp(dirAuxiliar->name, dir))
                {
                    if(debug == 1)
                    {
                        printf("Encontrado %s como subdiretorio de %s\n", dir, novoDiretorio->name);
                    }
                    encontrado = 1;
                    // se for igual encontrei
                    novoDiretorio = dirAuxiliar;
                    novoDiretorioBloco = *(novoDiretorio->dirFilhos+m);
                    // ele � o novoDiretorio, para procurar o prox�mo token nele
                }
                else
                {
                    m++;
                }
            }
            m=0; // para pr�xima itera��o

            // se acabou o loop e n�o encontrou, desaloca buffer e retorna null
            if (encontrado == 0)
            {
                if(debug==1)
                {
                    printf("Nao encontrado subdiretorio %s\n", dir);
                }
                free(buffer);
                return NULL;
            }

            dir = strtok(NULL, "/");
            encontrado = 0;
        }

        return novoDiretorio;
    }
}


